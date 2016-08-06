/*!
  @file shading.fp
	
  @brief GLSL�t���O�����g�V�F�[�_
 
  @author Makoto Fujisawa
  @date 2011
*/
// FILE --shading.fp--
#version 120


//-----------------------------------------------------------------------------
// �ϐ�
//-----------------------------------------------------------------------------
//
// �o�[�e�b�N�X�V�F�[�_����󂯎��ϐ�
//
varying vec4 vPos;
varying vec3 vNrm;


//
// GL����ݒ肳���萔(uniform)
//
uniform float fresnelBias;
uniform float fresnelScale; 
uniform float fresnelPower; 
uniform float etaRatio;
uniform vec3 eyePosition;
uniform samplerCube envmap;


//-----------------------------------------------------------------------------
// �֐�
//-----------------------------------------------------------------------------
vec4 lerp(vec4 a, vec4 b, float s)
{
	return vec4(a+(b-a)*s);       
}

vec3 lerp(vec3 a, vec3 b, float s)
{
	return vec3(a+(b-a)*s);       
}

//-----------------------------------------------------------------------------
// ���˃��f���֐�
//-----------------------------------------------------------------------------
/*!
 * Fresnel���˃��f���ɂ��V�F�[�f�B���O
 * @return �\�ʔ��ːF
 */
vec4 FresnelShading(void)
{
	// ���ˁC���ˁC���܃x�N�g���̌v�Z
	vec3 N = normalize(vNrm);			// �@���x�N�g��
	vec3 I = normalize(vPos.xyz-eyePosition);		// ���˃x�N�g��
	vec3 R = reflect(I, N);			// ���˃x�N�g��
	vec3 T = refract(I, N, etaRatio);	// ���܃x�N�g��

	// ���ˈ����̌v�Z
	float fresnel = fresnelBias+fresnelScale*pow(min(0.0, 1.0-dot(I, N)), fresnelPower);

	// ���ˊ��F�̎擾
	vec4 reflecColor = textureCube(envmap, R);
	reflecColor.a = 1.0;

	// ���܊��F�̌v�Z
	vec4 refracColor;
	refracColor.rgb = textureCube(envmap, T).rgb;
	refracColor.a = 1.0;

	// �F�𓝍�
	vec4 cout = lerp(refracColor, reflecColor, fresnel);
	cout.a = fresnel*0.5+0.5;

	return cout;
}



//-----------------------------------------------------------------------------
// �G���g���֐�
//-----------------------------------------------------------------------------
void main(void)
{	
	// �\�ʔ��ːF
	gl_FragColor = FresnelShading();
}

