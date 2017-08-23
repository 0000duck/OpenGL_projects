/*! 
 @file rx_shader.h

 @brief GLSL�V�F�[�_�[
 
 @author Makoto Fujisawa
 @date 2009-11
*/
// FILE --rx_shader.h--


#ifndef _RX_SHADERS_H_
#define _RX_SHADERS_H_


//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
#include <cstdio>
#include <iostream>
#include <vector>
#include <string>

#include <GL/glew.h>
#include <GL/glut.h>


#define RXSTR(A) #A

using namespace std;


//-----------------------------------------------------------------------------
// HACK:GLSL�V�F�[�_
//-----------------------------------------------------------------------------
struct rxGLSL
{
	string VertProg;
	string FragProg;
	string Name;
	GLuint Prog;
};

//-----------------------------------------------------------------------------
// Volume Rendering �V�F�[�_
//-----------------------------------------------------------------------------
const char volume_vs[] = RXSTR(
varying vec3 texcoord;	// �{�����[���e�N�X�`���p�̃e�N�X�`�����W
void main(void)
{
	gl_Position = gl_ProjectionMatrix*gl_Vertex;
	texcoord = (gl_ModelViewMatrixInverse*gl_Vertex).xyz;
	gl_ClipVertex = gl_Vertex;
}
);

const char volume_fs[] = RXSTR(
uniform sampler3D volume_tex;
uniform float thickness;	// �X���C�X�ԋ���(=�X���C�X����)
uniform float opacity;		// �����x
uniform vec3 dens_col;		// �{�����[���̐F
varying vec3 texcoord;
void main(void)
{
	float d = texture3D(volume_tex, texcoord).x;
	gl_FragColor = vec4(dens_col.r, dens_col.g, dens_col.b, d*opacity*thickness);
	//gl_FragColor = vec4(d, 1.0, 1.0, 0.5);
}
);


//-----------------------------------------------------------------------------
// Shadow Map �V�F�[�_
//-----------------------------------------------------------------------------
//MARK:Shadow Map vs
const char shadow_vs[] = RXSTR(
varying vec4 vPos;
varying vec3 vNrm;
void main(void)
{
	vPos = gl_ModelViewMatrix*gl_Vertex;
	vNrm = normalize(gl_NormalMatrix*gl_Normal);
	//vNrm = gl_NormalMatrix*gl_Normal;

	gl_Position = gl_ProjectionMatrix*vPos;
	//gl_FrontColor = gl_Color*gl_LightSource[0].diffuse*vec4(max(dot(vNrm, gl_LightSource[0].position.xyz), 0.0));
	gl_FrontColor = gl_Color;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	//gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

}
);

// MARK:Normal Mode fs
const char shadow_single_fs[] = RXSTR( 
#version 110 @
#extension GL_EXT_texture_array : enable @

uniform sampler2D tex;			//!< �͗l
uniform sampler2DArray stex;	//!< �f�v�X�l�e�N�X�`��(�~�����䕪����)
uniform vec4 far_d;				//!< �����䉓����
varying vec4 vPos;
varying vec3 vNrm;

float ShadowCoef(void)
{
	// ��f�ʒugl_FragCoord.xy �ɂ�����f�v�X�l gl_FragCoord.z����K�؂ȃf�v�X�}�b�v������
	// �������ȏ�̕����ɂ�0�������Ă�������(�Ⴆ�Ε�����2�Ȃ�,far_d.z,far_d.w��0)
	int index = 3;
	if(gl_FragCoord.z < far_d.x){
		index = 0;
	}
	else if(gl_FragCoord.z < far_d.y){
		index = 1;
	}
	else if(gl_FragCoord.z < far_d.z){
		index = 2;
	}
	
	// ���_���W�n�̈ʒu�����������_�Ƃ������W�n�̂��̂ɕϊ�
	vec4 shadow_coord = gl_TextureMatrix[index]*vPos;

	// ��������̃f�v�X�l(�Օ�����)��Ҕ�
	float light_d = shadow_coord.z;
	
	// �ǂ̕�����p���邩
	shadow_coord.z = float(index);
	
	// �i�[���ꂽ��������̍ŏ��f�v�X�l���擾
	float shadow_d = texture2DArray(stex, shadow_coord.xyz).x;
	
	// ��������̃f�v�X�l�ƎՕ����l�������f�v�X�l�̍������߂�
	float diff = shadow_d-light_d;

	// �e��0,������1��Ԃ�
	return clamp(diff*250.0+1.0, 0.0, 1.0);
}

void main(void)
{
	vec4 light_col;
	vec3 N = normalize(vNrm);
	//vec3 L = gl_LightSource[0].position.xyz;
	vec3 L = normalize(gl_LightSource[0].position.xyz-vPos.xyz);	// �����x�N�g��

	// �����̌v�Z
	//  - OpenGL���v�Z�����������x�Ɣ��ˌW���̐�(gl_FrontLightProduct)��p����D
	light_col = gl_FrontLightProduct[0].ambient;

	// �g�U���˂̌v�Z
	//float diff = max(dot(L, N), 0.0);
	float diff = max(dot(vNrm, gl_LightSource[0].position.xyz), 0.0);

	// ���ʔ��˂̌v�Z
	if(diff > 0.0){
		// ���˃x�N�g���̌v�Z
		vec3 V = normalize(-vPos.xyz);
		//vec3 R = 2.0*dot(N, L)*N-L;
		vec3 R = reflect(-L, N);
		float spec = pow(abs(dot(R, V)), gl_FrontMaterial.shininess);

		light_col += gl_FrontLightProduct[0].diffuse*diff+
					 gl_FrontLightProduct[0].specular*spec;
	}

	const float shadow_ambient = 0.8;
	vec4 color_tex = texture2D(tex, gl_TexCoord[0].st);
	float shadow_coef = ShadowCoef();

	light_col += color_tex;

	gl_FragColor = shadow_ambient*shadow_coef*light_col+(1.0-shadow_ambient)*color_tex;
	//gl_FragColor = shadow_ambient*shadow_coef*gl_Color+(1.0-shadow_ambient)*gl_Color;
	//gl_FragColor = shadow_ambient*shadow_coef*light_col*color_tex+(1.0-shadow_ambient)*color_tex;

}
);


//! �����̃V���h�E�T���v�����g�p(���[�N�}����)
// MARK:Show Splits fs
const char shadow_single_hl_fs[] = RXSTR(
#version 120 @
#extension GL_EXT_texture_array : enable @

uniform sampler2D tex;			//!< �͗l
uniform sampler2DArray stex;	//!< �f�v�X�l�e�N�X�`��(�~�����䕪����)
uniform vec4 far_d;				//!< �����䉓����
uniform vec4 color[4] = vec4[4](vec4(1.0, 0.5, 0.5, 1.0),
								vec4(0.5, 1.0, 0.5, 1.0),
								vec4(0.5, 0.5, 1.0, 1.0),
								vec4(1.0, 1.0, 0.5, 1.0));

// �o�[�e�b�N�X�V�F�[�_����󂯎��ϐ�
varying vec4 vPos;
varying vec3 vNrm;

vec4 ShadowCoef(void)
{
	// ��f�ʒugl_FragCoord.xy �ɂ�����f�v�X�l gl_FragCoord.z����K�؂ȃf�v�X�}�b�v������
	// �������ȏ�̕����ɂ�0�������Ă�������(�Ⴆ�Ε�����2�Ȃ�,far_d.z,far_d.w��0)
	int index = 3;
	if(gl_FragCoord.z < far_d.x){
		index = 0;
	}
	else if(gl_FragCoord.z < far_d.y){
		index = 1;
	}
	else if(gl_FragCoord.z < far_d.z){
		index = 2;
	}
	
	// ���_���W�n�̈ʒu�����������_�Ƃ������W�n�̂��̂ɕϊ�
	vec4 shadow_coord = gl_TextureMatrix[index]*vPos;

	// ��������̃f�v�X�l(�Օ�����)��Ҕ�
	float light_d = shadow_coord.z;
	
	// �ǂ̕�����p���邩
	shadow_coord.z = float(index);
	
	// �i�[���ꂽ��������̍ŏ��f�v�X�l���擾
	float shadow_d = texture2DArray(stex, shadow_coord.xyz).x;
	
	// ��������̃f�v�X�l�ƎՕ����l�������f�v�X�l�̍������߂�
	float diff = shadow_d-light_d;

	// �e��0,������1��Ԃ�
	return clamp(diff*250.0+1.0, 0.0, 1.0)*color[index];
}

void main(void)
{
	const float shadow_ambient = 0.9;
	vec4 color_tex = texture2D(tex, gl_TexCoord[0].st);
	vec4 shadow_coef = ShadowCoef();
	gl_FragColor = shadow_ambient*shadow_coef*gl_Color+(1.0-shadow_ambient)*gl_Color;
}
);


//! �����̃V���h�E�T���v�����g�p
// MARK:Smooth shadows fs
const char shadow_multi_fs[] = RXSTR(
#version 120 @
#extension GL_EXT_texture_array : enable @

uniform sampler2D tex;			//!< �͗l
uniform sampler2DArray stex;	//!< �f�v�X�l�e�N�X�`��(�~�����䕪����)
uniform vec4 far_d;				//!< �����䉓����

// �o�[�e�b�N�X�V�F�[�_����󂯎��ϐ�
varying vec4 vPos;
varying vec3 vNrm;

// ���͂̉e�W���Q�Ɨp
const int nsamples = 8;
uniform vec4 offset[nsamples] = vec4[nsamples](vec4(0.000000, 0.000000, 0.0, 0.0),
											   vec4(0.079821, 0.165750, 0.0, 0.0),
											   vec4(-0.331500, 0.159642, 0.0, 0.0),
											   vec4(-0.239463, -0.497250, 0.0, 0.0),
											   vec4(0.662999, -0.319284, 0.0, 0.0),
											   vec4(0.399104, 0.828749, 0.0, 0.0),
											   vec4(-0.994499, 0.478925, 0.0, 0.0),
											   vec4(-0.558746, -1.160249, 0.0, 0.0));

/*!
 * �e�W���̌v�Z
 * @param[in] shadow_coord �V���h�E�}�b�v�Q�Ɨp�e�N�X�`�����W
 * @param[in] light_d �Q�ƈʒu�ł̌�������̃f�v�X�l
 * @return �e�W��(�e�̂���Ƃ����1, ����ȊO��0)
 */
float GetOccCoef(vec4 shadow_coord, float light_d)
{
	// �i�[���ꂽ��������̍ŏ��f�v�X�l���擾
	float shadow_d = texture2DArray(stex, shadow_coord.xyz).x;
	
	// ��������̃f�v�X�l�ƎՕ����l�������f�v�X�l�̍������߂�
	float diff = shadow_d-light_d;
	
	// �e��0,������1��Ԃ�
	return clamp(diff*250.0+1.0, 0.0, 1.0);
}

/*!
 * �e�����̂��߂̌W��(�e�̂���Ƃ����1, ����ȊO��0)
 * @return �e�W��(�e�̂���Ƃ����1, ����ȊO��0)
 */
float ShadowCoef(void)
{
	// ��f�ʒugl_FragCoord.xy �ɂ�����f�v�X�l gl_FragCoord.z����K�؂ȃf�v�X�}�b�v������
	// �������ȏ�̕����ɂ�0�������Ă�������(�Ⴆ�Ε�����2�Ȃ�,far_d.z,far_d.w��0)
	int index = 3;
	if(gl_FragCoord.z < far_d.x){
		index = 0;
	}
	else if(gl_FragCoord.z < far_d.y){
		index = 1;
	}
	else if(gl_FragCoord.z < far_d.z){
		index = 2;
	}
	
	const float scale = 2.0/4096.0;

	// ���_���W�n�̈ʒu�����������_�Ƃ������W�n�̂��̂ɕϊ�
	vec4 shadow_coord = gl_TextureMatrix[index]*vPos;
	
	// ��������̃f�v�X�l(�Օ�����)��Ҕ�
	float light_d = shadow_coord.z;
	
	// �ǂ̕�����p���邩
	shadow_coord.z = float(index);
	
	// ���͂̉e�W�����܂߂Ď擾
	float shadow_coef = 0.0;
	for(int i = 0; i < nsamples; ++i){
		shadow_coef += GetOccCoef(shadow_coord+scale*offset[i], light_d);
	}
	shadow_coef /= nsamples;
	
	return shadow_coef;
}

void main(void)
{
	vec4 light_col;
	vec3 N = normalize(vNrm);
	//vec3 L = gl_LightSource[0].position.xyz;
	vec3 L = normalize(gl_LightSource[0].position.xyz-vPos.xyz);	// �����x�N�g��

	// �����̌v�Z
	//  - OpenGL���v�Z�����������x�Ɣ��ˌW���̐�(gl_FrontLightProduct)��p����D
	light_col = gl_FrontLightProduct[0].ambient;

	// �g�U���˂̌v�Z
	//float diff = max(dot(L, N), 0.0);
	float diff = max(dot(vNrm, gl_LightSource[0].position.xyz), 0.0);

	// ���ʔ��˂̌v�Z
	if(diff > 0.0){
		// ���˃x�N�g���̌v�Z
		vec3 V = normalize(-vPos.xyz);
		//vec3 R = 2.0*dot(N, L)*N-L;
		vec3 R = reflect(-L, N);
		float spec = pow(abs(dot(R, V)), gl_FrontMaterial.shininess);

		light_col += gl_FrontLightProduct[0].diffuse*diff+
					 gl_FrontLightProduct[0].specular*spec;
	}

	const float shadow_ambient = 0.8;
	vec4 color_tex = texture2D(tex, gl_TexCoord[0].st);
	float shadow_coef = ShadowCoef();

	gl_FragColor = shadow_ambient*shadow_coef*light_col+(1.0-shadow_ambient)*light_col;
	//gl_FragColor = shadow_ambient*shadow_coef*gl_Color+(1.0-shadow_ambient)*gl_Color;
}
);


//! �����̃V���h�E�T���v�����g�p(���[�N�}����)
// MARK:Smooth shadows, no leak fs
const char shadow_multi_noleak_fs[] = RXSTR(
#version 120 @
#extension GL_EXT_texture_array : enable @

uniform sampler2D tex;			//!< �͗l
uniform sampler2DArray stex;	//!< �f�v�X�l�e�N�X�`��(�~�����䕪����)
uniform vec4 far_d;				//!< �����䉓����

// �o�[�e�b�N�X�V�F�[�_����󂯎��ϐ�
varying vec4 vPos;
varying vec3 vNrm;

// ���͂̉e�W���Q�Ɨp
const int nsamples = 8;
uniform vec4 offset[nsamples] = vec4[nsamples](vec4(0.000000, 0.000000, 0.0, 0.0),
											   vec4(0.079821, 0.165750, 0.0, 0.0),
											   vec4(-0.331500, 0.159642, 0.0, 0.0),
											   vec4(-0.239463, -0.497250, 0.0, 0.0),
											   vec4(0.662999, -0.319284, 0.0, 0.0),
											   vec4(0.399104, 0.828749, 0.0, 0.0),
											   vec4(-0.994499, 0.478925, 0.0, 0.0),
											   vec4(-0.558746, -1.160249, 0.0, 0.0));

/*!
 * �e�W���̌v�Z
 * @param[in] shadow_coord �V���h�E�}�b�v�Q�Ɨp�e�N�X�`�����W
 * @param[in] light_d �Q�ƈʒu�ł̌�������̃f�v�X�l
 * @return �e�W��(�e�̂���Ƃ����1, ����ȊO��0)
 */
float GetOccCoef(vec4 shadow_coord, float light_d)
{
	// �i�[���ꂽ��������̍ŏ��f�v�X�l���擾
	float shadow_d = texture2DArray(stex, shadow_coord.xyz).x;
	
	// ��������̃f�v�X�l�ƎՕ����l�������f�v�X�l�̍������߂�
	float diff = shadow_d-light_d;
	
	// �e��0,������1��Ԃ�
	return clamp(diff*250.0+1.0, 0.0, 1.0);
}

/*!
 * �e�����̂��߂̌W��(�e�̂���Ƃ����1, ����ȊO��0)
 * @return �e�W��(�e�̂���Ƃ����1, ����ȊO��0)
 */
float ShadowCoef(void)
{
	// ��f�ʒugl_FragCoord.xy �ɂ�����f�v�X�l gl_FragCoord.z����K�؂ȃf�v�X�}�b�v������
	// �������ȏ�̕����ɂ�0�������Ă�������(�Ⴆ�Ε�����2�Ȃ�,far_d.z,far_d.w��0)
	int index = 3;
	if(gl_FragCoord.z < far_d.x){
		index = 0;
	}
	else if(gl_FragCoord.z < far_d.y){
		index = 1;
	}
	else if(gl_FragCoord.z < far_d.z){
		index = 2;
	}
	
	const float scale = 2.0/4096.0;

	// ���_���W�n�̈ʒu�����������_�Ƃ������W�n�̂��̂ɕϊ�
	vec4 shadow_coord = gl_TextureMatrix[index]*vPos;

	vec4 light_normal4 = gl_TextureMatrix[index+4]*vec4(vNrm, 0.0);
	vec3 light_normal = normalize(light_normal4.xyz);
	
	float d = -dot(light_normal, shadow_coord.xyz);

	// ��������̃f�v�X�l(�Օ�����)��Ҕ�
	float light_d = shadow_coord.z;
	
	// �ǂ̕�����p���邩
	shadow_coord.z = float(index);
	
	// ���͂̉e�W�����܂߂Ď擾
	float shadow_coef = GetOccCoef(shadow_coord, light_d);
	for(int i = 1; i < nsamples; ++i){
		vec4 shadow_lookup = shadow_coord+scale*offset[i];

		float lookup_z = -(light_normal.x*shadow_lookup.x + light_normal.y*shadow_lookup.y + d)/light_normal.z;

		shadow_coef += GetOccCoef(shadow_lookup, lookup_z);
	}
	shadow_coef /= nsamples;
	
	return shadow_coef;
}

void main(void)
{
	vec4 light_col;
	vec3 N = normalize(vNrm);

	//vec3 L = gl_LightSource[0].position.xyz;
	vec3 L = normalize(gl_LightSource[0].position.xyz-vPos.xyz);	// �����x�N�g��

	// �����̌v�Z
	//  - OpenGL���v�Z�����������x�Ɣ��ˌW���̐�(gl_FrontLightProduct)��p����D
	light_col = gl_FrontLightProduct[0].ambient;

	// �g�U���˂̌v�Z
	//float diff = max(dot(L, N), 0.0);
	float diff = max(dot(vNrm, gl_LightSource[0].position.xyz), 0.0);

	// ���ʔ��˂̌v�Z
	if(diff > 0.0){
		// ���˃x�N�g���̌v�Z
		vec3 V = normalize(-vPos.xyz);
		//vec3 R = 2.0*dot(N, L)*N-L;
		vec3 R = reflect(-L, N);
		float spec = pow(abs(dot(R, V)), gl_FrontMaterial.shininess);

		light_col += gl_FrontLightProduct[0].diffuse*diff+
					 gl_FrontLightProduct[0].specular*spec;
	}

	const float shadow_ambient = 0.9;
	vec4 color_tex = texture2D(tex, gl_TexCoord[0].st);
	float shadow_coef = ShadowCoef();

	//light_col += color_tex;

	gl_FragColor = shadow_ambient*shadow_coef*light_col+(1.0-shadow_ambient)*light_col;
	//gl_FragColor = shadow_ambient*shadow_coef*gl_Color+(1.0-shadow_ambient)*gl_Color;
}
);


// sampler2DArrayShadow���g�p
// MARK:PCF fs
const char shadow_pcf_fs[] = RXSTR(
#version 110 @
#extension GL_EXT_texture_array : enable @

uniform sampler2D tex;				//!< �͗l
uniform sampler2DArrayShadow stex;	//!< �f�v�X�l�e�N�X�`��(�~�����䕪����)
uniform vec4 far_d;					//!< �����䉓����
uniform vec2 texSize;				//!< x - size, y - 1/size

// �o�[�e�b�N�X�V�F�[�_����󂯎��ϐ�
varying vec4 vPos;
varying vec3 vNrm;

float ShadowCoef(void)
{
	// ��f�ʒugl_FragCoord.xy �ɂ�����f�v�X�l gl_FragCoord.z����K�؂ȃf�v�X�}�b�v������
	// �������ȏ�̕����ɂ�0�������Ă�������(�Ⴆ�Ε�����2�Ȃ�,far_d.z,far_d.w��0)
	int index = 3;
	if(gl_FragCoord.z < far_d.x){
		index = 0;
	}
	else if(gl_FragCoord.z < far_d.y){
		index = 1;
	}
	else if(gl_FragCoord.z < far_d.z){
		index = 2;
	}
	
	// ���_���W�n�̈ʒu�����������_�Ƃ������W�n�̂��̂ɕϊ�
	vec4 shadow_coord = gl_TextureMatrix[index]*vPos;

	// ��������̃f�v�X�l(�Օ�����)��Ҕ�
	shadow_coord.w = shadow_coord.z;
	
	// �ǂ̕�����p���邩
	shadow_coord.z = float(index);
	
	// �e��0,������1��Ԃ�
	return shadow2DArray(stex, shadow_coord).x;
}

void main(void)
{
	vec4 light_col;
	vec3 N = normalize(vNrm);
	//vec3 L = gl_LightSource[0].position.xyz;
	vec3 L = normalize(gl_LightSource[0].position.xyz-vPos.xyz);	// �����x�N�g��

	// �����̌v�Z
	//  - OpenGL���v�Z�����������x�Ɣ��ˌW���̐�(gl_FrontLightProduct)��p����D
	light_col = gl_FrontLightProduct[0].ambient;

	// �g�U���˂̌v�Z
	//float diff = max(dot(L, N), 0.0);
	float diff = max(dot(vNrm, gl_LightSource[0].position.xyz), 0.0);

	// ���ʔ��˂̌v�Z
	if(diff > 0.0){
		// ���˃x�N�g���̌v�Z
		vec3 V = normalize(-vPos.xyz);
		//vec3 R = 2.0*dot(N, L)*N-L;
		vec3 R = reflect(-L, N);
		float spec = pow(abs(dot(R, V)), gl_FrontMaterial.shininess);

		light_col += gl_FrontLightProduct[0].diffuse*diff+
					 gl_FrontLightProduct[0].specular*spec;
	}

	const float shadow_ambient = 0.8;
	vec4 color_tex = texture2D(tex, gl_TexCoord[0].st);
	float shadow_coef = ShadowCoef();

	gl_FragColor = shadow_ambient*shadow_coef*light_col+(1.0-shadow_ambient)*light_col;
	//gl_FragColor = shadow_ambient*shadow_coef*gl_Color+(1.0-shadow_ambient)*gl_Color;
}
);


// sampler2DArrayShadow���g�p
// MARK:PCF w/ trilinear fs
const char shadow_pcf_trilinear_fs[] = RXSTR(
#version 110 @
#extension GL_EXT_texture_array : enable @

uniform sampler2D tex;				//!< �͗l
uniform sampler2DArrayShadow stex;	//!< �f�v�X�l�e�N�X�`��(�~�����䕪����)
uniform vec4 far_d;					//!< �����䉓����
uniform vec2 texSize;				//!< x - size, y - 1/size

// �o�[�e�b�N�X�V�F�[�_����󂯎��ϐ�
varying vec4 vPos;
varying vec3 vNrm;

float ShadowCoef(void)
{
	// ��f�ʒugl_FragCoord.xy �ɂ�����f�v�X�l gl_FragCoord.z����K�؂ȃf�v�X�}�b�v������
	// �������ȏ�̕����ɂ�0�������Ă�������(�Ⴆ�Ε�����2�Ȃ�,far_d.z,far_d.w��0)
	int index = 3;
	float blend = 0.0;
	if(gl_FragCoord.z < far_d.x){
		index = 0;
		blend = clamp( (gl_FragCoord.z-far_d.x*0.995)*200.0, 0.0, 1.0); 
	}
	else if(gl_FragCoord.z < far_d.y){
		index = 1;
		blend = clamp( (gl_FragCoord.z-far_d.y*0.995)*200.0, 0.0, 1.0); 
	}
	else if(gl_FragCoord.z < far_d.z){
		index = 2;
		blend = clamp( (gl_FragCoord.z-far_d.z*0.995)*200.0, 0.0, 1.0); 
	}
	
	// ���_���W�n�̈ʒu�����������_�Ƃ������W�n�̂��̂ɕϊ�
	vec4 shadow_coord = gl_TextureMatrix[index]*vPos;

	// ��������̃f�v�X�l(�Օ�����)��Ҕ�
	shadow_coord.w = shadow_coord.z;
	
	// �ǂ̕�����p���邩
	shadow_coord.z = float(index);
	
	// �e�W���̎擾
	float ret = shadow2DArray(stex, shadow_coord).x;
	
	if(blend > 0.0){
		shadow_coord = gl_TextureMatrix[index+1]*vPos;
	
		shadow_coord.w = shadow_coord.z;
		shadow_coord.z = float(index+1);
		
		ret = ret*(1.0-blend) + shadow2DArray(stex, shadow_coord).x*blend; 
	}
	
	return ret;
}

void main(void)
{
	vec4 light_col;
	vec3 N = normalize(vNrm);
	//vec3 L = gl_LightSource[0].position.xyz;
	vec3 L = normalize(gl_LightSource[0].position.xyz-vPos.xyz);	// �����x�N�g��

	// �����̌v�Z
	//  - OpenGL���v�Z�����������x�Ɣ��ˌW���̐�(gl_FrontLightProduct)��p����D
	light_col = gl_FrontLightProduct[0].ambient;

	// �g�U���˂̌v�Z
	//float diff = max(dot(L, N), 0.0);
	float diff = max(dot(vNrm, gl_LightSource[0].position.xyz), 0.0);

	// ���ʔ��˂̌v�Z
	if(diff > 0.0){
		// ���˃x�N�g���̌v�Z
		vec3 V = normalize(-vPos.xyz);
		//vec3 R = 2.0*dot(N, L)*N-L;
		vec3 R = reflect(-L, N);
		float spec = pow(abs(dot(R, V)), gl_FrontMaterial.shininess);

		light_col += gl_FrontLightProduct[0].diffuse*diff+
					 gl_FrontLightProduct[0].specular*spec;
	}

	const float shadow_ambient = 0.8;
	vec4 color_tex = texture2D(tex, gl_TexCoord[0].st);
	float shadow_coef = ShadowCoef();

	gl_FragColor = shadow_ambient*shadow_coef*light_col+(1.0-shadow_ambient)*light_col;
	//gl_FragColor = shadow_ambient*shadow_coef*gl_Color+(1.0-shadow_ambient)*gl_Color;
}
);


//! 
// MARK:PCF w/ 4 taps fs
const char shadow_pcf_4tap_fs[] = RXSTR(
#version 120 @
#extension GL_EXT_texture_array : enable @

uniform sampler2D tex;				//!< �͗l
uniform sampler2DArrayShadow stex;	//!< �f�v�X�l�e�N�X�`��(�~�����䕪����)
uniform vec4 far_d;					//!< �����䉓����
uniform vec2 texSize;				//!< x - size, y - 1/size

// �o�[�e�b�N�X�V�F�[�_����󂯎��ϐ�
varying vec4 vPos;
varying vec3 vNrm;

float ShadowCoef(void)
{
	// ��f�ʒugl_FragCoord.xy �ɂ�����f�v�X�l gl_FragCoord.z����K�؂ȃf�v�X�}�b�v������
	// �������ȏ�̕����ɂ�0�������Ă�������(�Ⴆ�Ε�����2�Ȃ�,far_d.z,far_d.w��0)
	int index = 3;
	if(gl_FragCoord.z < far_d.x){
		index = 0;
	}
	else if(gl_FragCoord.z < far_d.y){
		index = 1;
	}
	else if(gl_FragCoord.z < far_d.z){
		index = 2;
	}
	
	// ���_���W�n�̈ʒu�����������_�Ƃ������W�n�̂��̂ɕϊ�
	vec4 shadow_coord = gl_TextureMatrix[index]*vPos;

	// ��������̃f�v�X�l(�Օ�����)��Ҕ�
	shadow_coord.w = shadow_coord.z;
	
	// �ǂ̕�����p���邩
	shadow_coord.z = float(index);

	// �d�ݕt��4-tap�o�C���j�A�t�B���^
	vec2 pos = mod(shadow_coord.xy*texSize.x, 1.0);
	vec2 offset = (0.5-step(0.5, pos))*texSize.y;
	float ret = 0.0;
	ret += shadow2DArray(stex, shadow_coord+vec4( offset.x,  offset.y, 0, 0)).x * (pos.x) * (pos.y);
	ret += shadow2DArray(stex, shadow_coord+vec4( offset.x, -offset.y, 0, 0)).x * (pos.x) * (1-pos.y);
	ret += shadow2DArray(stex, shadow_coord+vec4(-offset.x,  offset.y, 0, 0)).x * (1-pos.x) * (pos.y);
	ret += shadow2DArray(stex, shadow_coord+vec4(-offset.x, -offset.y, 0, 0)).x * (1-pos.x) * (1-pos.y);
	
	return ret;
}

void main(void)
{
	vec4 light_col;
	vec3 N = normalize(vNrm);
	//vec3 L = gl_LightSource[0].position.xyz;
	vec3 L = normalize(gl_LightSource[0].position.xyz-vPos.xyz);	// �����x�N�g��

	// �����̌v�Z
	//  - OpenGL���v�Z�����������x�Ɣ��ˌW���̐�(gl_FrontLightProduct)��p����D
	light_col = gl_FrontLightProduct[0].ambient;

	// �g�U���˂̌v�Z
	//float diff = max(dot(L, N), 0.0);
	float diff = max(dot(vNrm, gl_LightSource[0].position.xyz), 0.0);

	// ���ʔ��˂̌v�Z
	if(diff > 0.0){
		// ���˃x�N�g���̌v�Z
		vec3 V = normalize(-vPos.xyz);
		//vec3 R = 2.0*dot(N, L)*N-L;
		vec3 R = reflect(-L, N);
		float spec = pow(abs(dot(R, V)), gl_FrontMaterial.shininess);

		light_col += gl_FrontLightProduct[0].diffuse*diff+
					 gl_FrontLightProduct[0].specular*spec;
	}

	const float shadow_ambient = 0.8;
	vec4 color_tex = texture2D(tex, gl_TexCoord[0].st);
	float shadow_coef = ShadowCoef();

	gl_FragColor = shadow_ambient*shadow_coef*light_col+(1.0-shadow_ambient)*light_col;
	//gl_FragColor = shadow_ambient*shadow_coef*gl_Color+(1.0-shadow_ambient)*gl_Color;
}
);


//! 
// MARK:PCF w/ 8 random taps fs
const char shadow_pcf_8tap_fs[] = RXSTR(
#version 120 @
#extension GL_EXT_texture_array : enable @

uniform sampler2D tex;				//!< �͗l
uniform sampler2DArrayShadow stex;	//!< �f�v�X�l�e�N�X�`��(�~�����䕪����)
uniform vec4 far_d;					//!< �����䉓����
uniform vec2 texSize;				//!< x - size, y - 1/size

// �o�[�e�b�N�X�V�F�[�_����󂯎��ϐ�
varying vec4 vPos;
varying vec3 vNrm;

// ���͂̉e�W���Q�Ɨp
const int nsamples = 8;
uniform vec4 offset[nsamples] = vec4[nsamples](vec4(0.000000, 0.000000, 0.0, 0.0),
											   vec4(0.079821, 0.165750, 0.0, 0.0),
											   vec4(-0.331500, 0.159642, 0.0, 0.0),
											   vec4(-0.239463, -0.497250, 0.0, 0.0),
											   vec4(0.662999, -0.319284, 0.0, 0.0),
											   vec4(0.399104, 0.828749, 0.0, 0.0),
											   vec4(-0.994499, 0.478925, 0.0, 0.0),
											   vec4(-0.558746, -1.160249, 0.0, 0.0));

float ShadowCoef(void)
{
	// ��f�ʒugl_FragCoord.xy �ɂ�����f�v�X�l gl_FragCoord.z����K�؂ȃf�v�X�}�b�v������
	// �������ȏ�̕����ɂ�0�������Ă�������(�Ⴆ�Ε�����2�Ȃ�,far_d.z,far_d.w��0)
	int index = 3;
	if(gl_FragCoord.z < far_d.x){
		index = 0;
	}
	else if(gl_FragCoord.z < far_d.y){
		index = 1;
	}
	else if(gl_FragCoord.z < far_d.z){
		index = 2;
	}
	
	// ���_���W�n�̈ʒu�����������_�Ƃ������W�n�̂��̂ɕϊ�
	vec4 shadow_coord = gl_TextureMatrix[index]*vPos;

	// ��������̃f�v�X�l(�Օ�����)��Ҕ�
	shadow_coord.w = shadow_coord.z;
	
	// �ǂ̕�����p���邩
	shadow_coord.z = float(index);

	// �d�ݕt��8-tap�����_���t�B���^
	float ret = 0.0;
	for(int i = 0; i < nsamples; ++i){
		vec4 shadow_lookup = shadow_coord+texSize.y*offset[i]*2.0; //scale the offsets to the texture size, and make them twice as large to cover a larger radius
		ret += shadow2DArray(stex, shadow_lookup).x*0.125;
	}
	
	return ret;
}

void main(void)
{
	vec4 light_col;
	vec3 N = normalize(vNrm);
	//vec3 L = gl_LightSource[0].position.xyz;
	vec3 L = normalize(gl_LightSource[0].position.xyz-vPos.xyz);	// �����x�N�g��

	// �����̌v�Z
	//  - OpenGL���v�Z�����������x�Ɣ��ˌW���̐�(gl_FrontLightProduct)��p����D
	light_col = gl_FrontLightProduct[0].ambient;

	// �g�U���˂̌v�Z
	//float diff = max(dot(L, N), 0.0);
	float diff = max(dot(vNrm, gl_LightSource[0].position.xyz), 0.0);

	// ���ʔ��˂̌v�Z
	if(diff > 0.0){
		// ���˃x�N�g���̌v�Z
		vec3 V = normalize(-vPos.xyz);
		//vec3 R = 2.0*dot(N, L)*N-L;
		vec3 R = reflect(-L, N);
		float spec = pow(abs(dot(R, V)), gl_FrontMaterial.shininess);

		light_col += gl_FrontLightProduct[0].diffuse*diff+
					 gl_FrontLightProduct[0].specular*spec;
	}

	const float shadow_ambient = 0.8;
	vec4 color_tex = texture2D(tex, gl_TexCoord[0].st);
	float shadow_coef = ShadowCoef();

	gl_FragColor = shadow_ambient*shadow_coef*light_col+(1.0-shadow_ambient)*light_col;
	//gl_FragColor = shadow_ambient*shadow_coef*gl_Color+(1.0-shadow_ambient)*gl_Color;
}
);


//! 
// MARK:PCF w/ gaussian blur fs
const char shadow_pcf_gaussian_fs[] = RXSTR(
#version 110 @
#extension GL_EXT_texture_array : enable @

uniform sampler2D tex;				//!< �͗l
uniform sampler2DArrayShadow stex;	//!< �f�v�X�l�e�N�X�`��(�~�����䕪����)
uniform vec4 far_d;					//!< �����䉓����
uniform vec2 texSize;				//!< x - size, y - 1/size

// �o�[�e�b�N�X�V�F�[�_����󂯎��ϐ�
varying vec4 vPos;
varying vec3 vNrm;

float ShadowCoef(void)
{
	// ��f�ʒugl_FragCoord.xy �ɂ�����f�v�X�l gl_FragCoord.z����K�؂ȃf�v�X�}�b�v������
	// �������ȏ�̕����ɂ�0�������Ă�������(�Ⴆ�Ε�����2�Ȃ�,far_d.z,far_d.w��0)
	int index = 3;
	if(gl_FragCoord.z < far_d.x){
		index = 0;
	}
	else if(gl_FragCoord.z < far_d.y){
		index = 1;
	}
	else if(gl_FragCoord.z < far_d.z){
		index = 2;
	}
	
	// ���_���W�n�̈ʒu�����������_�Ƃ������W�n�̂��̂ɕϊ�
	vec4 shadow_coord = gl_TextureMatrix[index]*vPos;

	// ��������̃f�v�X�l(�Օ�����)��Ҕ�
	shadow_coord.w = shadow_coord.z;
	
	// �ǂ̕�����p���邩
	shadow_coord.z = float(index);

	// Gaussian 3x3 filter
	float ret = shadow2DArray(stex, shadow_coord).x * 0.25;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( -1, -1)).x * 0.0625;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( -1, 0)).x * 0.125;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( -1, 1)).x * 0.0625;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( 0, -1)).x * 0.125;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( 0, 1)).x * 0.125;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( 1, -1)).x * 0.0625;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( 1, 0)).x * 0.125;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( 1, 1)).x * 0.0625;
	
	return ret;
}

void main(void)
{
	vec4 light_col;
	vec3 N = normalize(vNrm);
	//vec3 L = gl_LightSource[0].position.xyz;
	vec3 L = normalize(gl_LightSource[0].position.xyz-vPos.xyz);	// �����x�N�g��

	// �����̌v�Z
	//  - OpenGL���v�Z�����������x�Ɣ��ˌW���̐�(gl_FrontLightProduct)��p����D
	light_col = gl_FrontLightProduct[0].ambient;

	// �g�U���˂̌v�Z
	//float diff = max(dot(L, N), 0.0);
	float diff = max(dot(vNrm, gl_LightSource[0].position.xyz), 0.0);

	// ���ʔ��˂̌v�Z
	if(diff > 0.0){
		// ���˃x�N�g���̌v�Z
		vec3 V = normalize(-vPos.xyz);
		//vec3 R = 2.0*dot(N, L)*N-L;
		vec3 R = reflect(-L, N);
		float spec = pow(abs(dot(R, V)), gl_FrontMaterial.shininess);

		light_col += gl_FrontLightProduct[0].diffuse*diff+
					 gl_FrontLightProduct[0].specular*spec;
	}

	const float shadow_ambient = 0.8;
	vec4 color_tex = texture2D(tex, gl_TexCoord[0].st);
	float shadow_coef = ShadowCoef();

	gl_FragColor = shadow_ambient*shadow_coef*light_col+(1.0-shadow_ambient)*light_col;
	//gl_FragColor = shadow_ambient*shadow_coef*gl_Color+(1.0-shadow_ambient)*gl_Color;
}
);










//-----------------------------------------------------------------------------
// Shadow View �V�F�[�_
//-----------------------------------------------------------------------------
const char shadow_view_vs[] = RXSTR(
void main(void)
{
	gl_TexCoord[0] = vec4(0.5)*gl_Vertex + vec4(0.5);
	gl_Position = gl_Vertex;
}
);

const char shadow_view_fs[] = RXSTR(
#version 110 @
#extension GL_EXT_texture_array : enable @

uniform sampler2DArray tex;
uniform float layer;

void main(void)
{
	vec4 tex_coord = vec4(gl_TexCoord[0].x, gl_TexCoord[0].y, layer, 1.0);
	gl_FragColor = texture2DArray(tex, tex_coord.xyz);
//	gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
);


//-----------------------------------------------------------------------------
// MARK:Kajiya-kai �V�F�[�_
//-----------------------------------------------------------------------------

//! Kajiya-kai���f�� ���_�V�F�[�_
const char kajiyakai_vs[] = RXSTR(
varying vec4 vPos;
varying vec3 vNrm;
void main(void)
{
	vPos = gl_ModelViewMatrix*gl_Vertex;
	vNrm = normalize(gl_NormalMatrix*gl_Normal);

	gl_Position = ftransform();
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_ModelViewMatrix * gl_Vertex;

	// ���_->���_�x�N�g��
	vec3 E = normalize(vPos.xyz);
	vec3 L = normalize(gl_LightSource[0].position.xyz-vPos.xyz);	// �����x�N�g��

	// Kajiya-kay���f��
	vec3 nl = cross(vNrm, L);		// N�~L
	float nnl = sqrt(dot(nl, nl));	// |N�~L| = sin��

	vec3 ne = cross(vNrm, E);		// N�~E
	float nne = sqrt(dot(ne, ne));	// |N�~E| = sin��

	float dnl = dot(vNrm, L);
	float dne = dot(vNrm, E);

	float spec = dne*dnl+nne*nnl;
	//spec *= spec;	// 2��
	//spec *= spec;	// 4��
	//spec *= spec;	// 8��
	spec = pow(max(spec, 0.0), 80.0f);
	
	// Kajiya-kay�g�U��
	vec3 Ad = gl_FrontMaterial.diffuse.xyz*nnl;		// Kd sin�� = Kd |n�~l|
	vec3 As = gl_FrontMaterial.specular.xyz*spec;	// Ks (cos��)^n = Ks(cos��cos��-sin��sin��)^n

	// �����o�[�g�g�U
	float diff = max(0.0, dot(L, vNrm));
	vec3 Ld = gl_FrontLightProduct[0].diffuse.xyz*diff;

	gl_FrontColor.rgb = Ad+As+Ld;

	//gl_FrontColor.rgb = 0.5 * gl_Normal.xyz + 0.5;
	gl_FrontColor.a = 1.0;
}
);

//! Kajiya-kai���f�� �s�N�Z���V�F�[�_
const char kajiyakai_fs[] = RXSTR(
varying vec4 vPos;
varying vec3 vNrm;
void main(void)
{
	gl_FragColor = gl_Color;
}
);




//-----------------------------------------------------------------------------
// MARK:toon�V�F�[�_
//-----------------------------------------------------------------------------

//! �g�D�[�������_�����O ���_�V�F�[�_
const char toon_vs[] = RXSTR(
#version 110 @
#extension GL_EXT_texture_array : enable @

uniform vec3 lightPosition;
uniform vec3 eyePosition;
uniform float shininess;

// �t���O�����g�V�F�[�_�ɒl��n�����߂̕ϐ�
varying vec4 vPos;
varying vec3 vNrm;
varying float fLightDiff;
varying float fLightSpec;
varying float fEdge;

void main(void)
{
	// ���_�ʒu�Ɩ@��
	vPos = gl_ModelViewMatrix*gl_Vertex;
	vNrm = normalize(gl_NormalMatrix*gl_Normal);

	gl_Position = ftransform();
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_ModelViewMatrix * gl_Vertex;

	// Calculate diffuse lighting
	vec3 N = normalize(vNrm);
	vec3 L = normalize(lightPosition-gl_Vertex.xyz);
	fLightDiff = max(0.0, dot(L, N));

	// Calculate specular lighting
	vec3 V = normalize(eyePosition-gl_Vertex.xyz);
	vec3 H = normalize(L+V);
	fLightSpec = pow(max(0.0, dot(H, N)), shininess);
	if(fLightSpec <= 0.0) fLightSpec = 0.0;

	// Perform edge detection
	fEdge = max(0.0, dot(V, N));	
}
);

//! �g�D�[�������_�����O �s�N�Z���V�F�[�_
const char toon_fs[] = RXSTR(
#version 110 @
#extension GL_EXT_texture_array : enable @

uniform vec4 Kd;
uniform vec4 Ks;
//uniform sampler1D texDiffRamp;
//uniform sampler1D texSpecRamp;
//uniform sampler1D texEdgeRamp;

// �o�[�e�b�N�X�V�F�[�_����󂯎��ϐ�
varying vec4 vPos;
varying vec3 vNrm;
varying float fLightDiff;
varying float fLightSpec;
varying float fEdge;

void main(void)
{
//	fLightDiff = texture1D(texDiffRamp, fLightDiff).x;
//	fLightSpec = texture1D(texSpecRamp, fLightSpec).x;
//	fEdge      = texture1D(texEdgeRamp, fEdge).x;
	float ldiff = (fLightDiff > 0.75) ? 0.75 : ((fLightDiff > 0.5) ? 0.5 : 0.1);
	float lspec = (fLightSpec > 0.5) ? 1.0 : 0.0;
	float edge  = (fEdge > 0.5) ? 1.0 : 0.0;

	gl_FragColor = edge*(Kd*ldiff+Ks*lspec);
}
);



//-----------------------------------------------------------------------------
// MARK:Phong�V�F�[�_
//-----------------------------------------------------------------------------

//! Phong ���_�V�F�[�_
const char phong_vs[] = RXSTR(
#version 110 @
#extension GL_EXT_texture_array : enable @

// �t���O�����g�V�F�[�_�ɒl��n�����߂̕ϐ�
varying vec4 vPos;
varying vec3 vNrm;
varying vec3 vObjPos;

void main(void)
{
	// ���_�ʒu�Ɩ@��
	vPos = gl_ModelViewMatrix*gl_Vertex;
	vNrm = normalize(gl_NormalMatrix*gl_Normal);

	gl_Position = ftransform();
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_ModelViewMatrix * gl_Vertex;
	
	vObjPos = gl_Vertex.xyz;
}
);

//! Phong �s�N�Z���V�F�[�_
const char phong_fs[] = RXSTR(
#version 110 @
#extension GL_EXT_texture_array : enable @

uniform vec3 Ke; // ���ːF
uniform vec3 Ka; // ����
uniform vec3 Kd; // �g�U����
uniform vec3 Ks; // ���ʔ���
uniform float shine;
uniform vec3 La;	// ���C�g����
uniform vec3 Ld;	// ���C�g�g�U���ˌ�
uniform vec3 Ls;	// ���C�g���ʔ��ˌ�
uniform vec3 Lpos;	// ���C�g�ʒu
uniform vec3 eyePosition;

// �o�[�e�b�N�X�V�F�[�_����󂯎��ϐ�
varying vec4 vPos;
varying vec3 vNrm;
varying vec3 vObjPos;

void main(void)
{
	vec3 P = vObjPos.xyz;
	vec3 N = normalize(vNrm);
	
	// ���ːF�̌v�Z
	vec3 emissive = Ke;

	// �����̌v�Z
	vec3 ambient = Ka*La;

	// �g�U���˂̌v�Z
	vec3 L = normalize(Lpos-P);
	float diffuseLight = max(dot(L, N), 0.0);
	vec3 diffuse = Kd*Ld*diffuseLight;

	// ���ʔ��˂̌v�Z
	vec3 V = normalize(eyePosition-P);
	vec3 H = normalize(L+V);
	float specularLight = pow(max(dot(H, N), 0.0), shine);
	if(diffuseLight <= 0.0) specularLight = 0.0;
	vec3 specular = Ks*Ls*specularLight;

	gl_FragColor.xyz = emissive+ambient+diffuse+specular;
	gl_FragColor.w = 1.0;
}
);

//-----------------------------------------------------------------------------
// MARK:Fresnel�V�F�[�_
//-----------------------------------------------------------------------------

//! Fresnel ���_�V�F�[�_
const char fresnel_vs[] = RXSTR(
#version 120 @
#extension GL_EXT_texture_array : enable @

uniform float fresnelBias;
uniform float fresnelScale; 
uniform float fresnelPower; 
uniform float etaRatio;
uniform vec3 eyePosition;

// �t���O�����g�V�F�[�_�ɒl��n�����߂̕ϐ�
varying vec4 vPos;
varying vec3 vNrm;
varying float fNoise;
varying vec3 oR;
varying vec3 oT;
varying float oFresnel;

void main(void)
{
	// ���_�ʒu�Ɩ@��
	fNoise = gl_Vertex.w;
	vec4 vert = gl_Vertex;
	vert.w = 1.0f;

	vPos = gl_ModelViewMatrix*vert;
	vNrm = normalize(gl_NormalMatrix*gl_Normal);

	gl_Position = gl_ModelViewProjectionMatrix*vert;//ftransform();
	gl_TexCoord[0] = gl_TextureMatrix[0]*gl_ModelViewMatrix*vert;

	// ���ˁC���ˁC���܃x�N�g���̌v�Z
	vec3 I = vPos.xyz-eyePosition;
	I = normalize(I);
	oR = reflect(I, vNrm);
	oT = refract(I, vNrm, etaRatio);

	// ���ˈ����̌v�Z
	oFresnel = fresnelBias+fresnelScale*pow(min(0.0, 1.0-dot(I, vNrm)), fresnelPower);
}
);

//! Fresnel �s�N�Z���V�F�[�_
const char fresnel_fs[] = RXSTR(
#version 120 @
#extension GL_EXT_texture_array : enable @

uniform samplerCube envmap;

uniform float maxNoise;
uniform float minNoise;
uniform vec3 FoamColor;

// �o�[�e�b�N�X�V�F�[�_����󂯎��ϐ�
varying vec4 vPos;
varying vec3 vNrm;
varying float fNoise;
varying vec3 oR;
varying vec3 oT;
varying float oFresnel;

vec4 lerp(vec4 a, vec4 b, float s)
{
	return vec4(a + (b - a) * s);       
}

vec3 lerp(vec3 a, vec3 b, float s)
{
	return vec3(a + (b - a) * s);       
}

void main(void)
{
	// ���ˊ��F�̎擾
	vec4 reflecColor = textureCube(envmap, oR);
	reflecColor.a = 1.0;

	// ���܊��F�̌v�Z
	vec4 refracColor;
	refracColor.rgb = textureCube(envmap, oT).rgb;
	refracColor.a = 1.0;

	vec4 cout = lerp(refracColor, reflecColor, oFresnel);

	if(fNoise > minNoise){
		float foam_rate = (fNoise-minNoise)/(maxNoise-minNoise);
		gl_FragColor.rgb = lerp(cout.rgb, FoamColor, foam_rate);
	}
	else{
		gl_FragColor.rgb = cout.rgb;
	}	

	gl_FragColor.a = oFresnel*0.5+0.5;
}
);



//-----------------------------------------------------------------------------
// MARK:pointsprite�V�F�[�_
//-----------------------------------------------------------------------------

// vertex shader
const char ps_vs[] = RXSTR(
uniform float pointRadius;  // �|�C���g�T�C�Y
uniform float pointScale;   // �s�N�Z���X�P�[���ɕϊ�����Ƃ��̔{��
uniform float zCrossFront;
uniform float zCrossBack;
varying float vValid;
void main(void)
{
	if(gl_Vertex.z > zCrossFront || gl_Vertex.z < zCrossBack){
		vValid = 0.0;
	}
	else{
		vValid = 1.0;
	}

	// �E�B���h�E�X�y�[�X�ł̃|�C���g�T�C�Y���v�Z
	vec3 posEye = vec3(gl_ModelViewMatrix * vec4(gl_Vertex.xyz, 1.0));
	float dist = length(posEye);
	gl_PointSize = pointRadius*(pointScale/dist);

	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0);

	gl_FrontColor = gl_Color;
}
);

// pixel shader for rendering points as shaded spheres
const char ps_fs[] = RXSTR(
varying float vValid;
uniform vec3 lightDir;	// ��������
void main(void)
{
	if(vValid < 0.5) discard;

	//const vec3 lightDir = vec3(0.577, 0.577, 0.577);

	// �e�N�X�`�����W����@�����v�Z(���Ƃ��ĕ`��)
	vec3 N;
	N.xy = gl_TexCoord[0].xy*vec2(2.0, -2.0)+vec2(-1.0, 1.0);

	float mag = dot(N.xy, N.xy);	// ���S�����2�拗��
	if(mag > 1.0) discard;   // �~�̊O�̃s�N�Z���͔j��
	N.z = sqrt(1.0-mag);

	// ���������Ɩ@������\�ʐF���v�Z
	float diffuse = max(0.0, dot(lightDir, N));

	gl_FragColor = gl_Color*diffuse;
}
);


//-----------------------------------------------------------------------------
// MARK:pointsprite2d�V�F�[�_
//-----------------------------------------------------------------------------
// vertex shader
const char ps2d_vs[] = RXSTR(
uniform float pointRadius;  // point size in world space
uniform float pointScale;   // scale to calculate size in pixels
uniform float densityScale;
uniform float densityOffset;
void main(void)
{
	// calculate window-space point size
	gl_PointSize = pointRadius*pointScale;

	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0);

	gl_FrontColor = gl_Color;
}
);

// pixel shader for rendering points as shaded spheres
const char ps2d_fs[] = RXSTR(
void main(void)
{
	// calculate normal from texture coordinates
	vec3 N;
	N.xy = gl_TexCoord[0].xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);
	float mag = dot(N.xy, N.xy);
	if (mag > 1.0) discard;   // kill pixels outside circle

	gl_FragColor = gl_Color;
}
);




//-----------------------------------------------------------------------------
// MARK:GLSL�R���p�C��
//-----------------------------------------------------------------------------
/*!
 * GLSL�v���O�����̃R���p�C��
 * @param[in] vsource vertex�V�F�[�_�v���O�������e
 * @param[in] fsource pixel(fragment)�V�F�[�_�v���O�������e
 * @return GLSL�v���O�����ԍ�
 */
static GLuint CompileProgram(const char *vsource, const char *fsource)
{
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertexShader, 1, &vsource, 0);
	glShaderSource(fragmentShader, 1, &fsource, 0);
	
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);

	GLuint program = glCreateProgram();

	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	glLinkProgram(program);

	// check if program linked
	GLint success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (!success) {
		char temp[256];
		glGetProgramInfoLog(program, 256, 0, temp);
		printf("Failed to link program:\n%s\n", temp);
		glDeleteProgram(program);
		program = 0;
	}

	return program;
}

/*!
 * GLSL�V�F�[�_�R���p�C��
 * @param[in] target �^�[�Q�b�g(GL_VERTEX_SHADER,GL_FRAGMENT_SHADER)
 * @param[in] shader �V�F�[�_�R�[�h
 * @return GLSL�I�u�W�F�N�g
 */
inline GLuint CompileGLSLShader(GLenum target, const char* shader)
{
	// GLSL�I�u�W�F�N�g�쐬
	GLuint object = glCreateShader(target);

	if(!object) return 0;

	glShaderSource(object, 1, &shader, NULL);
	glCompileShader(object);

	// �R���p�C����Ԃ̊m�F
	GLint compiled = 0;
	glGetShaderiv(object, GL_COMPILE_STATUS, &compiled);

	if(!compiled){
		char temp[256] = "";
		glGetShaderInfoLog( object, 256, NULL, temp);
		fprintf(stderr, " Compile failed:\n%s\n", temp);

		glDeleteShader(object);
		return 0;
	}

	return object;
}

/*!
 * GLSL�V�F�[�_�R���p�C��
 * @param[in] target �^�[�Q�b�g(GL_VERTEX_SHADER,GL_FRAGMENT_SHADER)
 * @param[in] fn �V�F�[�_�t�@�C���p�X
 * @return GLSL�I�u�W�F�N�g
 */
inline GLuint CompileGLSLShaderFromFile(GLenum target, const char* fn)
{
	FILE *fp;

	// �o�C�i���Ƃ��ăt�@�C���ǂݍ���
	fopen_s(&fp, fn, "rb");
	if(fp == NULL) return 0;

	// �t�@�C���̖����Ɉړ������݈ʒu(�t�@�C���T�C�Y)���擾
	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);

	fseek(fp, 0, SEEK_SET);

	// �V�F�[�_�̓��e�i�[
	char *text = new char[size+1];
	fread(text, size, 1, fp);
	text[size] = '\0';

	//printf("%s\n", text);


	fclose(fp);

	// �V�F�[�_�R���p�C��
	printf("Compile %s\n", fn);
	GLuint object = CompileGLSLShader(target, text);

	delete [] text;

	return object;
}

/*!
 * �o�[�e�b�N�X�ƃt���O�����g�V�F�[�_�ō\�������GLSL�v���O�����쐬
 * @param[in] vs �o�[�e�b�N�X�V�F�[�_�I�u�W�F�N�g
 * @param[in] fs �t���O�����g�V�F�[�_�I�u�W�F�N�g
 * @return GLSL�v���O�����I�u�W�F�N�g
 */
inline GLuint LinkGLSLProgram(GLuint vs, GLuint fs)
{
	// �v���O�����I�u�W�F�N�g�쐬
	GLuint program = glCreateProgram();

	// �V�F�[�_�I�u�W�F�N�g��o�^
	glAttachShader(program, vs);
	glAttachShader(program, fs);

	// �v���O�����̃����N
	glLinkProgram(program);

	// �G���[�o��
	GLint charsWritten, infoLogLength;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

	char * infoLog = new char[infoLogLength];
	glGetProgramInfoLog(program, infoLogLength, &charsWritten, infoLog);
	printf(infoLog);
	delete [] infoLog;

	// �����J�e�X�g
	GLint linkSucceed = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &linkSucceed);
	if(linkSucceed == GL_FALSE){
		glDeleteProgram(program);
		return 0;
	}

	return program;
}


/*!
 * �o�[�e�b�N�X/�W�I���g��/�t���O�����g�V�F�[�_�ō\�������GLSL�v���O�����쐬
 * @param[in] vs �o�[�e�b�N�X�V�F�[�_�I�u�W�F�N�g
 * @param[in] gs �W�I���g���V�F�[�_�I�u�W�F�N�g
 * @param[in] inputType �W�I���g���V�F�[�_�ւ̓��̓^�C�v
 * @param[in] vertexOut �o�[�e�b�N�X�̏o��
 * @param[in] outputType �W�I���g���V�F�[�_����̏o�̓^�C�v
 * @param[in] fs �t���O�����g�V�F�[�_�I�u�W�F�N�g
 * @return GLSL�v���O�����I�u�W�F�N�g
 */
inline GLuint LinkGLSLProgram(GLuint vs, GLuint gs, GLint inputType, GLint vertexOut, GLint outputType, GLuint fs)
{
	// �v���O�����I�u�W�F�N�g�쐬
	GLuint program = glCreateProgram();

	// �V�F�[�_�I�u�W�F�N�g��o�^
	glAttachShader(program, vs);
	glAttachShader(program, gs);

	glProgramParameteriEXT(program, GL_GEOMETRY_INPUT_TYPE_EXT, inputType);
	glProgramParameteriEXT(program, GL_GEOMETRY_VERTICES_OUT_EXT, vertexOut);
	glProgramParameteriEXT(program, GL_GEOMETRY_OUTPUT_TYPE_EXT, outputType);
	glAttachShader(program, fs);

	// �v���O�����̃����N
	glLinkProgram(program);

	// �G���[�o��
	GLint charsWritten, infoLogLength;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

	char * infoLog = new char[infoLogLength];
	glGetProgramInfoLog(program, infoLogLength, &charsWritten, infoLog);
	printf(infoLog);
	delete [] infoLog;

	// �����J�e�X�g
	GLint linkSucceed = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &linkSucceed);
	if(linkSucceed == GL_FALSE){
		glDeleteProgram(program);
		return 0;
	}

	return program;
}


/*!
 * GLSL�̃R���p�C���E�����N(�t�@�C�����)
 * @param[in] vs ���_�V�F�[�_�t�@�C���p�X
 * @param[in] fs �t���O�����g�V�F�[�_�t�@�C���p�X
 * @param[in] name �v���O������
 * @return GLSL�I�u�W�F�N�g
 */
inline rxGLSL CreateGLSLFromFile(const string &vs, const string &fs, const string &name)
{
	rxGLSL gs;
	gs.VertProg = vs;
	gs.FragProg = fs;
	gs.Name = name;

	GLuint v, f;
	printf("compile the vertex shader : %s\n", name.c_str());
	if(!(v = CompileGLSLShaderFromFile(GL_VERTEX_SHADER, vs.c_str()))){
		// skip the first three chars to deal with path differences
		v = CompileGLSLShaderFromFile(GL_VERTEX_SHADER, &vs.c_str()[3]);
	}

	printf("compile the fragment shader : %s\n", name.c_str());
	if(!(f = CompileGLSLShaderFromFile(GL_FRAGMENT_SHADER, fs.c_str()))){
		f = CompileGLSLShaderFromFile(GL_FRAGMENT_SHADER, &fs.c_str()[3]);
	}

	gs.Prog = LinkGLSLProgram(v, f);
	//gs.Prog = GLSL_CreateShaders(gs.VertProg.c_str(), gs.FragProg.c_str());

	return gs;
}

/*!
 * #version�Ȃǂ̃v���v���Z�b�T�𕶎���Ƃ��ď����ꂽ�V�F�[�_���Ɋ܂ޏꍇ�C���s�����܂������Ȃ��̂ŁC
 *  #version 110 @ �̂悤�ɍŌ��@��t���C���s�ɕϊ�����
 * @param[in] s  �V�F�[�_������
 * @param[in] vs �ϊ���̃V�F�[�_������
 */
inline void CreateGLSLShaderString(const char* s, vector<char> &vs)
{
	int idx = 0;
	char c = s[0];
	while(c != '\0'){
		if(c == '@') c = '\n'; // #version�Ȃǂ��\�ɂ��邽�߂�@�����s�ɕϊ�

		vs.push_back(c);
		idx++;
		c = s[idx];
	}
	vs.push_back('\0');
}

/*!
 * GLSL�̃R���p�C���E�����N(��������)
 * @param[in] vs ���_�V�F�[�_���e
 * @param[in] fs �t���O�����g�V�F�[�_���e
 * @param[in] name �v���O������
 * @return GLSL�I�u�W�F�N�g
 */
inline rxGLSL CreateGLSL(const char* vs, const char* fs, const string &name)
{
	rxGLSL gs;
	gs.VertProg = "from char";
	gs.FragProg = "from char";
	gs.Name = name;

	vector<char> vs1, fs1;
	CreateGLSLShaderString(vs, vs1);
	CreateGLSLShaderString(fs, fs1);
	
	//printf("vertex shader : %d\n%s\n", vs1.size(), &vs1[0]);
	//printf("pixel shader  : %d\n%s\n", fs1.size(), &fs1[0]);

	GLuint v, f;
	printf("compile the vertex shader : %s\n", name.c_str());
	v = CompileGLSLShader(GL_VERTEX_SHADER, &vs1[0]);
	printf("compile the fragment shader : %s\n", name.c_str());
	f = CompileGLSLShader(GL_FRAGMENT_SHADER, &fs1[0]);
	gs.Prog = LinkGLSLProgram(v, f);

	return gs;
}



#endif // #ifndef _RX_SHADERS_H_