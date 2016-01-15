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



//-----------------------------------------------------------------------------
// �V���h�E�}�b�s���O�N���X
//-----------------------------------------------------------------------------
class rxShadowMap
{
	GLuint m_iFBODepth;		//!< �������猩���Ƃ��̃f�v�X���i�[����Framebuffer object
	GLuint m_iTexDepth;		//!< m_iFBODepth��attach����e�N�X�`��
	double m_fDepthSize[2];	//!< �f�v�X���i�[����e�N�X�`���̃T�C�Y

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
		m_fDepthSize[0] = w;
		m_fDepthSize[1] = h;
	
		// �f�v�X�l�e�N�X�`��
		glActiveTexture(GL_TEXTURE0);
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
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_fDepthSize[0], m_fDepthSize[1], 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	

		// FBO�쐬
		glGenFramebuffersEXT(1, &m_iFBODepth);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_iFBODepth);

		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	
		// �f�v�X�}�b�v�e�N�X�`����FBO�ɐڑ�
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_iTexDepth, 0);
	
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	/*!
	 * �v���W�F�N�V�����s��C���_�ʒu�̐ݒ�
	 * @param[in] f ������
	 */
	void SetFrustum(const rxFrustum &f)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(f.FOV, (double)f.W/(double)f.H, f.Near, f.Far);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		gluLookAt(f.Origin[0], f.Origin[1], f.Origin[2], f.LookAt[0], f.LookAt[1], f.LookAt[2], f.Up[0], f.Up[1], f.Up[2]);
	}

	/*!
	 * �V���h�E�}�b�v(�f�v�X�e�N�X�`��)�̍쐬
	 * @param[in] light ����
	 * @param[in] fpDraw �`��֐��|�C���^
	 */
	void MakeShadowMap(rxFrustum &light, void (*fpDraw)(void*), void* func_obj, bool self_shading = false)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_iFBODepth);	// FBO�Ƀ����_�����O
		glEnable(GL_TEXTURE_2D);	

		glUseProgram(0);

		// �r���[�|�[�g���V���h�E�}�b�v�̑傫���ɕύX
		glViewport(0, 0, m_fDepthSize[0], m_fDepthSize[1]);
	
		glClear(GL_DEPTH_BUFFER_BIT);
	
		// �f�v�X�l�ȊO�̐F�̃����_�����O�𖳌��ɂ���
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); 
	
		double light_proj[16];
		double light_modelview[16];
		light.W = m_fDepthSize[0];
		light.H = m_fDepthSize[1];
		SetFrustum(light);

		// �������_�̃��f���r���[�s��C�v���W�F�N�V�����s����擾
		glMatrixMode(GL_PROJECTION);
		glGetDoublev(GL_PROJECTION_MATRIX, light_proj);

		glMatrixMode(GL_MODELVIEW);
		glGetDoublev(GL_MODELVIEW_MATRIX, light_modelview);

		glPolygonOffset(1.1f, 4.0f);
		glEnable(GL_POLYGON_OFFSET_FILL);
	
		glDisable(GL_LIGHTING);
		if(self_shading){
			glDisable(GL_CULL_FACE);
		}
		else{
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
		}
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
		glMultMatrixd(light_proj);
		glMultMatrixd(light_modelview);
	
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// �����ɂ����F�̃����_�����O��L���ɂ���
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); 
	}

	/*!
	 * �e�t���ŃV�[���`��
	 * @param[in] camera ���_
	 * @param[in] fpDraw �`��֐��̃|�C���^
	 */
	void RenderSceneWithShadow(rxFrustum &camera, void (*fpDraw)(void*), void* func_obj)
	{
		// ���_�ݒ�
		SetFrustum(camera);

		glEnable(GL_TEXTURE_2D);

		// �f�v�X�e�N�X�`����\��t��
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, m_iTexDepth);
		
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		fpDraw(func_obj);

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


#endif // #ifdef _RX_SHADOWMAP_H_