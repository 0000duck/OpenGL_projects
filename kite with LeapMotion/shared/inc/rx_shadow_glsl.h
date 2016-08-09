/*! 
  @file rx_shadowmap.h
	
  @brief 
 
  @author Makoto Fujisawa
  @date 2011-
*/
// FILE --rx_shadowmap--

#ifndef _RX_SHADOWMAP_H_
#define _RX_SHADOWMAP_H_


//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------

// OpenGL
#include <GL/glew.h>
#include <GL/glut.h>

#include "rx_utility.h"
#include "rx_shaders.h"

using namespace std;


//-----------------------------------------------------------------------------
// ������
//-----------------------------------------------------------------------------
struct rxFrustum
{
	double Near;
	double Far;
	double FOV;	// deg
	double W, H;
	Vec3 Origin;
	Vec3 LookAt;
	Vec3 Up;
};

rxFrustum CalFrustum(double fov_deg, double near_d, double far_d, int w, int h, 
					 Vec3 pos, Vec3 lookat = Vec3(0.0), Vec3 up = Vec3(0.0, 1.0, 0.0));
void SetFrustum(const rxFrustum &f);
bool CalInvMat4x4(const GLfloat m[16], GLfloat invm[16]);


//-----------------------------------------------------------------------------
// Shadow Map �V�F�[�_
//-----------------------------------------------------------------------------
const char shadow1_vs[] = RXSTR(
#version 120 @

// �t���O�����g�V�F�[�_�ɒl��n�����߂̕ϐ�
varying vec4 vPos;
varying vec3 vNrm;
varying vec4 vShadowCoord;	//!< �V���h�E�f�v�X�}�b�v�̎Q�Ɨp���W

void main(void)
{
	// �t���O�����g�V�F�[�_�ł̌v�Z�p(���f���r���[�ϊ��̂�)
	vPos = gl_ModelViewMatrix*gl_Vertex;			// ���_�ʒu
	vNrm = normalize(gl_NormalMatrix*gl_Normal);	// ���_�@��
	vShadowCoord = gl_TextureMatrix[7]*gl_ModelViewMatrix*gl_Vertex;	// �e�p���W�l(�������S���W)

	// �`��p
	gl_Position = gl_ProjectionMatrix*vPos;	// ���_�ʒu
	gl_FrontColor = gl_Color;				// ���_�F
	gl_TexCoord[0] = gl_MultiTexCoord0;		// ���_�e�N�X�`�����W
}
);

const char shadow1_fs[] = RXSTR(
#version 120 @

// �o�[�e�b�N�X�V�F�[�_����󂯎��ϐ�
varying vec4 vPos;
varying vec3 vNrm;
varying vec4 vShadowCoord;


// GL����ݒ肳���萔(uniform)
uniform sampler2D tex;			//!< �͗l
uniform sampler2D depth_tex;	//!< �f�v�X�l�e�N�X�`��

// �e�̔Z��
uniform float shadow_ambient;

/*!
 * Phong���˃��f���ɂ��V�F�[�f�B���O
 * @return �\�ʔ��ːF
 */
vec4 PhongShading(void)
{
	vec3 N = normalize(vNrm);			// �@���x�N�g��
	vec3 L = normalize(gl_LightSource[0].position.xyz-vPos.xyz);	// ���C�g�x�N�g��

	// �����̌v�Z
	//  - OpenGL���v�Z�����������x�Ɣ��ˌW���̐�(gl_FrontLightProduct)��p����D
	vec4 ambient = gl_FrontLightProduct[0].ambient;

	// �g�U���˂̌v�Z
	//float dcoef = max(dot(L, N), 0.0);
	float dcoef = max(dot(vNrm, gl_LightSource[0].position.xyz), 0.0);

	// ���ʔ��˂̌v�Z
	vec4 diffuse = vec4(0.0);
	vec4 specular = vec4(0.0);
	if(dcoef > 0.0){
		vec3 V = normalize(-vPos.xyz);		// �����x�N�g��

		// ���˃x�N�g���̌v�Z(�t�H�����˃��f��)
		vec3 R = reflect(-L, N);
		//vec3 R = 2.0*dot(N, L)*N-L;	// reflect�֐���p���Ȃ��ꍇ
		float specularLight = pow(max(dot(R, V), 0.0), gl_FrontMaterial.shininess);

		diffuse  = gl_FrontLightProduct[0].diffuse*dcoef;
		specular = gl_FrontLightProduct[0].specular*specularLight;
	}
	return ambient+diffuse+specular;
}

/*!
 * �e�����̂��߂̌W��(�e�̂���Ƃ����1, ����ȊO��0)
 * @return �e�W��(�e�̂���Ƃ����1, ����ȊO��0)
 */
float ShadowCoef(void)
{
	// �������W
	vec4 shadow_coord1 = vShadowCoord/vShadowCoord.w;

	// ��������̃f�v�X�l(���_)
	float view_d = shadow_coord1.z;//-0.0001;
	
	// �i�[���ꂽ��������̍ŏ��f�v�X�l���擾
	float light_d = texture2D(depth_tex, shadow_coord1.xy).x;

	// �e��0,������1
	float shadow_coef = 1.0;
	if(vShadowCoord.w > 0.0){
		shadow_coef = light_d < view_d ? 0.0 : 1.0;
	}

	return shadow_coef;
}

void main(void)
{	
	vec4 light_col = PhongShading();	// �\�ʔ��ːF
	float shadow_coef = ShadowCoef();	// �e�e���W��

	// �o��
	gl_FragColor = shadow_ambient*shadow_coef*light_col+(1.0-shadow_ambient)*light_col;
	//gl_FragColor = light_col;
}
);


//-----------------------------------------------------------------------------
// �V���h�E�}�b�s���O�N���X
//-----------------------------------------------------------------------------
class rxShadowMap
{
	GLuint m_iFBODepth;		//!< �������猩���Ƃ��̃f�v�X���i�[����Framebuffer object
	GLuint m_iTexDepth;		//!< m_iFBODepth��attach����e�N�X�`��
	double m_fDepthSize[2];	//!< �f�v�X���i�[����e�N�X�`���̃T�C�Y

	rxGLSL m_glslShading;	//!< GLSL�V�F�[�_

public:
	//! �f�t�H���g�R���X�g���N�^
	rxShadowMap()
	{
		m_iFBODepth = 0;
		m_iTexDepth = 0;
		m_fDepthSize[0] = m_fDepthSize[1] = 512;
	}

	//! �f�X�g���N�^
	~rxShadowMap(){}


	/*!
	 * �V���h�E�}�b�v�pFBO�̏�����
	 * @param[in] w,h  �V���h�E�}�b�v�̉𑜓x
	 */
	void InitShadow(int w, int h)
	{
		glewInit();
		if(!glewIsSupported("GL_ARB_depth_texture "
							"GL_ARB_shadow "
							)){
			cout << "ERROR: Support for necessary OpenGL extensions missing." << endl;
			return;
		}

		m_fDepthSize[0] = w;
		m_fDepthSize[1] = h;
	
		// �f�v�X�l�e�N�X�`��
		glActiveTexture(GL_TEXTURE7);
		glGenTextures(1, &m_iTexDepth);
		glBindTexture(GL_TEXTURE_2D, m_iTexDepth);

		// �e�N�X�`���p�����[�^�̐ݒ�
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		GLfloat border_color[4] = {1, 1, 1, 1};
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
	
		// �e�N�X�`���̈�̊m��(GL_DEPTH_COMPONENT��p����)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_fDepthSize[0], m_fDepthSize[1], 0, 
					 GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	

		// FBO�쐬
		glGenFramebuffersEXT(1, &m_iFBODepth);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_iFBODepth);

		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	
		// �f�v�X�}�b�v�e�N�X�`����FBO�ɐڑ�
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_iTexDepth, 0);
	
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// GLSL
		m_glslShading = CreateGLSL(shadow1_vs, shadow1_fs, "shadow");
	}


	/*!
	 * �e�t���ŃV�[���`��
	 * @param[in] light ����
	 * @param[in] fpDraw �`��֐��̃|�C���^
	 */
	void RenderSceneWithShadow(rxFrustum &light, void (*fpDraw)(void*), void* func_obj, bool self_shading = false)
	{
		float light_proj[16], camera_proj[16];
		float light_modelview[16], camera_modelview[16];

		//
		// ���݂̎��_�s��C�����s����擾 or �쐬
		//
		glMatrixMode(GL_PROJECTION);

		// ���_�v���W�F�N�V�����s��̎擾
		glGetFloatv(GL_PROJECTION_MATRIX, camera_proj);
		glPushMatrix();	// ���̃v���W�F�N�V�����s���ޔ������Ă���

		// �����v���W�F�N�V�����s��̐����Ǝ擾
		glLoadIdentity();
		gluPerspective(light.FOV, (double)light.W/(double)light.H, light.Near, light.Far);
		glGetFloatv(GL_PROJECTION_MATRIX, light_proj);
		
		glMatrixMode(GL_MODELVIEW);

		// ���_���f���r���[�s��̎擾
		glGetFloatv(GL_MODELVIEW_MATRIX, camera_modelview);
		glPushMatrix();	// ���̃��f���r���[�s���ޔ������Ă���

		// �������f���r���[�s��̐����Ǝ擾
		glLoadIdentity();
		gluLookAt(light.Origin[0], light.Origin[1], light.Origin[2], 
				  light.LookAt[0], light.LookAt[1], light.LookAt[2], 
				  light.Up[0], light.Up[1], light.Up[2]);
		glGetFloatv(GL_MODELVIEW_MATRIX, light_modelview);

		// ���̃r���[�|�[�g������Ŗ߂����߂Ɋm��
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);


		//
		// �������烌���_�����O���ăV���h�E�}�b�v�𐶐�
		//
		glBindFramebuffer(GL_FRAMEBUFFER, m_iFBODepth);	// FBO�Ƀ����_�����O

		// �J���[�C�f�v�X�o�b�t�@�̃N���A
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearDepth(1.0f);

		// �r���[�|�[�g���V���h�E�}�b�v�̑傫���ɕύX
		glViewport(0, 0, m_fDepthSize[0], m_fDepthSize[1]);
	
		// ���������_�Ƃ��Đݒ�
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(light_proj);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(light_modelview);
	
		// �f�v�X�l�ȊO�̐F�̃����_�����O�𖳌��ɂ���
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); 
	
		glPolygonOffset(1.1f, 40.0f);
		glEnable(GL_POLYGON_OFFSET_FILL);

		glEnable(GL_TEXTURE_2D);	
	
		glDisable(GL_LIGHTING);
		if(self_shading){
			glDisable(GL_CULL_FACE);
		}
		else{
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
		}

		glUseProgram(0);
		fpDraw(func_obj);

		glDisable(GL_POLYGON_OFFSET_FILL);
	
	
		const double bias[16] = { 0.5, 0.0, 0.0, 0.0, 
								  0.0, 0.5, 0.0, 0.0,
								  0.0, 0.0, 0.5, 0.0,
								  0.5, 0.5, 0.5, 1.0 };
	

	
		// �e�N�X�`�����[�h�Ɉڍs
		glMatrixMode(GL_TEXTURE);
		glActiveTexture(GL_TEXTURE7);
	
		glLoadIdentity();
		glLoadMatrixd(bias);
	
		// �������S���W�ƂȂ�悤�Ƀe�N�X�`���s���ݒ�
		// �e�N�X�`���ϊ��s��Ƀ��f���r���[�C�v���W�F�N�V������ݒ�
		glMultMatrixf(light_proj);
		glMultMatrixf(light_modelview);

		//// ���݂̃��f���r���[�̋t�s��������Ă���
		GLfloat camera_modelview_inv[16];
		CalInvMat4x4(camera_modelview, camera_modelview_inv);
		glMultMatrixf(camera_modelview_inv);

		// ��]�ƕ��s�ړ��ŕ����Čv�Z����ꍇ(�������̕�������)
		//GLfloat rot[16];
		//rot[0] = camera_modelview[0]; rot[1] = camera_modelview[1]; rot[2]  = camera_modelview[2];  rot[3]  = 0.0f;
		//rot[4] = camera_modelview[4]; rot[5] = camera_modelview[5]; rot[6]  = camera_modelview[6];  rot[7]  = 0.0f;
		//rot[8] = camera_modelview[8]; rot[9] = camera_modelview[9]; rot[10] = camera_modelview[10]; rot[11] = 0.0f;
		//rot[12] = 0.0f;               rot[13] = 0.0f;               rot[14] = 0.0f;                 rot[15] = 1.0f;
		//glMultTransposeMatrixf(rot);
		//glTranslatef(-camera_modelview[12], -camera_modelview[13], -camera_modelview[14]);
	
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// �����ɂ����F�̃����_�����O��L���ɂ���
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); 

		// ���̃r���[�|�[�g�s��ɖ߂�
		glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

		// �ޔ������Ă��������_�s������ɖ߂�
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();


		//
		// �J�������猩���Ƃ��̃V�[���`��
		// 
		glEnable(GL_TEXTURE_2D);

		// �f�v�X�e�N�X�`����\��t��
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, m_iTexDepth);
		
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		// GLSL�V�F�[�_���Z�b�g
		glUseProgram(m_glslShading.Prog);
		glUniform1i(glGetUniformLocation(m_glslShading.Prog, "depth_tex"), 7);
		glUniform1f(glGetUniformLocation(m_glslShading.Prog, "shadow_ambient"), 0.7f);

		fpDraw(func_obj);

		glUseProgram(0);
		glActiveTexture(GL_TEXTURE0);

		glBindTexture(GL_TEXTURE_2D, 0);

	}

	/*!
	 * �f�v�X�}�b�v���e�N�X�`���Ƃ��ĕ\��
	 * @param[in] w,h �E�B���h�E�T�C�Y
	 */
	void DrawDepthTex(int w, int h)
	{
		glUseProgram(0);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, w, 0, h, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		glDisable(GL_LIGHTING);
		glColor4f(1, 1, 1, 1);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_iTexDepth);
		glEnable(GL_TEXTURE_2D);

		glNormal3d(0, 0, -1);
		glBegin(GL_QUADS);
		glTexCoord2d(0, 0); glVertex3f(0.05*w,     0.05*h, 0);
		glTexCoord2d(1, 0); glVertex3f(0.05*w+100, 0.05*h, 0);
		glTexCoord2d(1, 1); glVertex3f(0.05*w+100, 0.05*h+100, 0);
		glTexCoord2d(0, 1); glVertex3f(0.05*w,     0.05*h+100, 0);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
};



/*!
 * ������̐���
 * @param[in] fov_deg ����p[deg]
 * @param[in] near_d,far_d ���������͈̔�
 * @param[in] w,h �E�B���h�E�T�C�Y
 * @param[in] pos ���_�ʒu
 * @param[in] lookat �����_�ʒu
 * @param[in] up �����
 * @return ������
 */
inline rxFrustum CalFrustum(double fov_deg, double near_d, double far_d, int w, int h, 
							Vec3 pos, Vec3 lookat, Vec3 up)
{
	rxFrustum f;
	f.Near = near_d;
	f.Far = far_d;
	f.FOV = fov_deg;
	f.W = w;
	f.H = h;
	f.Origin = pos;
	f.LookAt = lookat;
	f.Up = up;
	return f;
}

/*!
	* �v���W�F�N�V�����s��C���_�ʒu�̐ݒ�
	* @param[in] f ������
	*/
inline void SetFrustum(const rxFrustum &f)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(f.FOV, (double)f.W/(double)f.H, f.Near, f.Far);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gluLookAt(f.Origin[0], f.Origin[1], f.Origin[2], f.LookAt[0], f.LookAt[1], f.LookAt[2], f.Up[0], f.Up[1], f.Up[2]);
}

/*!
 * 4x4�s��̍s�񎮂̌v�Z
 *  | m[0]  m[1]  m[2]  m[3]  |
 *  | m[4]  m[5]  m[6]  m[7]  |
 *  | m[8]  m[9]  m[10] m[11] |
 *  | m[12] m[13] m[14] m[15] |
 * @param[in] m ���̍s��
 * @return �s�񎮂̒l
 */
inline double CalDetMat4x4(const GLfloat m[16])
{
	return m[0]*m[5]*m[10]*m[15]+m[0]*m[6]*m[11]*m[13]+m[0]*m[7]*m[9]*m[14]
		  +m[1]*m[4]*m[11]*m[14]+m[1]*m[6]*m[8]*m[15]+m[1]*m[7]*m[10]*m[12]
		  +m[2]*m[4]*m[9]*m[15]+m[2]*m[5]*m[11]*m[12]+m[2]*m[7]*m[8]*m[13]
		  +m[3]*m[4]*m[10]*m[13]+m[3]*m[5]*m[8]*m[14]+m[3]*m[6]*m[9]*m[12]
		  -m[0]*m[5]*m[11]*m[14]-m[0]*m[6]*m[9]*m[15]-m[0]*m[7]*m[10]*m[13]
		  -m[1]*m[4]*m[10]*m[15]-m[1]*m[6]*m[11]*m[12]-m[1]*m[7]*m[8]*m[14]
		  -m[2]*m[4]*m[11]*m[13]-m[2]*m[5]*m[8]*m[15]-m[2]*m[7]*m[9]*m[12]
		  -m[3]*m[4]*m[9]*m[14]-m[3]*m[5]*m[10]*m[12]-m[3]*m[6]*m[8]*m[13];
}
 
/*!
 * 4x4�s��̍s�񎮂̌v�Z
 *  | m[0]  m[1]  m[2]  m[3]  |
 *  | m[4]  m[5]  m[6]  m[7]  |
 *  | m[8]  m[9]  m[10] m[11] |
 *  | m[12] m[13] m[14] m[15] |
 * @param[in] m ���̍s��
 * @param[out] invm �t�s��
 * @return �t�s��̑���
 */
inline bool CalInvMat4x4(const GLfloat m[16], GLfloat invm[16])
{
	GLfloat det = CalDetMat4x4(m);
	if(fabs(det) < RX_FEQ_EPS){
		return false;
	}
	else{
		GLfloat inv_det = 1.0/det;
 
		invm[0]  = inv_det*(m[5]*m[10]*m[15]+m[6]*m[11]*m[13]+m[7]*m[9]*m[14]-m[5]*m[11]*m[14]-m[6]*m[9]*m[15]-m[7]*m[10]*m[13]);
		invm[1]  = inv_det*(m[1]*m[11]*m[14]+m[2]*m[9]*m[15]+m[3]*m[10]*m[13]-m[1]*m[10]*m[15]-m[2]*m[11]*m[13]-m[3]*m[9]*m[14]);
		invm[2]  = inv_det*(m[1]*m[6]*m[15]+m[2]*m[7]*m[13]+m[3]*m[5]*m[14]-m[1]*m[7]*m[14]-m[2]*m[5]*m[15]-m[3]*m[6]*m[13]);
		invm[3]  = inv_det*(m[1]*m[7]*m[10]+m[2]*m[5]*m[11]+m[3]*m[6]*m[9]-m[1]*m[6]*m[11]-m[2]*m[7]*m[9]-m[3]*m[5]*m[10]);
 
		invm[4]  = inv_det*(m[4]*m[11]*m[14]+m[6]*m[8]*m[15]+m[7]*m[10]*m[12]-m[4]*m[10]*m[15]-m[6]*m[11]*m[12]-m[7]*m[8]*m[14]);
		invm[5]  = inv_det*(m[0]*m[10]*m[15]+m[2]*m[11]*m[12]+m[3]*m[8]*m[14]-m[0]*m[11]*m[14]-m[2]*m[8]*m[15]-m[3]*m[10]*m[12]);
		invm[6]  = inv_det*(m[0]*m[7]*m[14]+m[2]*m[4]*m[15]+m[3]*m[6]*m[12]-m[0]*m[6]*m[15]-m[2]*m[7]*m[12]-m[3]*m[4]*m[14]);
		invm[7]  = inv_det*(m[0]*m[6]*m[11]+m[2]*m[7]*m[8]+m[3]*m[4]*m[10]-m[0]*m[7]*m[10]-m[2]*m[4]*m[11]-m[3]*m[6]*m[8]);
 
		invm[8]  = inv_det*(m[4]*m[9]*m[15]+m[5]*m[11]*m[12]+m[7]*m[8]*m[13]-m[4]*m[11]*m[13]-m[5]*m[8]*m[15]-m[7]*m[9]*m[12]);
		invm[9]  = inv_det*(m[0]*m[11]*m[13]+m[1]*m[8]*m[15]+m[3]*m[9]*m[12]-m[0]*m[9]*m[15]-m[1]*m[11]*m[12]-m[3]*m[8]*m[13]);
		invm[10] = inv_det*(m[0]*m[5]*m[15]+m[1]*m[7]*m[12]+m[3]*m[4]*m[13]-m[0]*m[7]*m[13]-m[1]*m[4]*m[15]-m[3]*m[5]*m[12]);
		invm[11] = inv_det*(m[0]*m[7]*m[9]+m[1]*m[4]*m[11]+m[3]*m[5]*m[8]-m[0]*m[5]*m[11]-m[1]*m[7]*m[8]-m[3]*m[4]*m[9]);
 
		invm[12] = inv_det*(m[4]*m[10]*m[13]+m[5]*m[8]*m[14]+m[6]*m[9]*m[12]-m[4]*m[9]*m[14]-m[5]*m[10]*m[12]-m[6]*m[8]*m[13]);
		invm[13] = inv_det*(m[0]*m[9]*m[14]+m[1]*m[10]*m[12]+m[2]*m[8]*m[13]-m[0]*m[10]*m[13]-m[1]*m[8]*m[14]-m[2]*m[9]*m[12]);
		invm[14] = inv_det*(m[0]*m[6]*m[13]+m[1]*m[4]*m[14]+m[2]*m[5]*m[12]-m[0]*m[5]*m[14]-m[1]*m[6]*m[12]-m[2]*m[4]*m[13]);
		invm[15] = inv_det*(m[0]*m[5]*m[10]+m[1]*m[6]*m[8]+m[2]*m[4]*m[9]-m[0]*m[6]*m[9]-m[1]*m[4]*m[10]-m[2]*m[5]*m[8]);
 
		return true;
	}
}


#endif // #ifdef _RX_SHADOWMAP_H_