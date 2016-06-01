/*! 
  @file rx_shader.h

  @brief GLSL�V�F�[�_�[
 
  @author Makoto Fujisawa
  @date 2009-11
*/


#ifndef _RX_SHADERS_H_
#define _RX_SHADERS_H_


//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
#include <cstdio>
#include <iostream>
#include <string>

#include <GL/glew.h>
#include <GL/glut.h>


using namespace std;


//-----------------------------------------------------------------------------
// HACK:GLSL�V�F�[�_
//-----------------------------------------------------------------------------
struct rxGLSL
{
	string VertProg;	//!< ���_�v���O�����t�@�C����
	string GeomProg;	//!< �W�I���g���v���O�����t�@�C����
	string FragProg;	//!< �t���O�����g�v���O�����t�@�C����
	string Name;		//!< �V�F�[�_��
	GLuint Prog;		//!< �V�F�[�_ID
};


#define RXSTR(A) #A


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