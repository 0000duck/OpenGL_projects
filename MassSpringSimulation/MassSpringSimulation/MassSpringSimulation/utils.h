/*! @file utils.h
	
	@brief �l�X�Ȋ֐�
 
	@author Makoto Fujisawa
	@date 2012
*/

#ifndef _RX_UTILS_H_
#define _RX_UTILS_H_

#ifdef _DEBUG
#pragma comment(lib, "libjpegd.lib")
#pragma comment(lib, "libpngd.lib")
#pragma comment(lib, "zlibd.lib")
#pragma comment(lib, "rx_modeld.lib")
#else
#pragma comment(lib, "libjpeg.lib")
#pragma comment(lib, "libpng.lib")
#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "rx_model.lib")
#endif

#pragma comment(lib, "glew32.lib")


#pragma warning (disable: 4101)

//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
#include <fstream>
#include <iostream>
#include <vector>
#include <string>

#include <direct.h>


// OpenGL
#include <GL/glew.h>
#include <GL/glut.h>

// �l�X�Ȋ֐�
#include "rx_utility.h"

// �}�E�X�ɂ�鎋�_�ړ��ƃ}�E�X�s�b�N
#include "rx_trackball.h"
#include "rx_pick.h"

// �e�N�X�`�����摜���o��
#include "rx_gltexture.h"


using namespace std;

//-----------------------------------------------------------------------------
// �萔�E��`
//-----------------------------------------------------------------------------
#define RXCOUT std::cout



//-----------------------------------------------------------------------------
// �֐�
//-----------------------------------------------------------------------------
/*!
 * �p�X����t�@�C��������菜�����p�X�𒊏o
 * @param[in] path �p�X
 * @return �t�H���_�p�X
 */
inline string GetFolderPath(const string &path)
{
	size_t pos1;
 
	pos1 = path.rfind('\\');
	if(pos1 != string::npos){
		return path.substr(0, pos1+1);
		
	}
 
	pos1 = path.rfind('/');
	if(pos1 != string::npos){
		return path.substr(0, pos1+1);
	}
 
	return "";
}

/*!
 * �t�@�C��������
 * @param head : ��{�t�@�C����
 * @param ext  : �g���q
 * @param n    : �A��
 * @param d    : �A�Ԍ���
 * @return ���������t�@�C����
 */
static inline string CreateFileName(const string &head, const string &ext, int n, const int &d)
{
	string file_name = head;
	int dn = d-1;
	if(n > 0){
		dn = (int)(log10((double)n))+1;
	}
	else if(n == 0){
		dn = 1;
	}
	else{
		n = 0;
		dn = 1;
	}

	for(int i = 0; i < d-dn; ++i){
		file_name += "0";
	}

	file_name += RX_TO_STRING(n);
	if(!ext.empty() && ext[0] != '.') file_name += ".";
	file_name += ext;

	return file_name;
}

/*!
 * �p�X����g���q���������ɂ��Ď��o��
 * @param[in] path �t�@�C���p�X
 * @return (������������)�g���q
 */
inline string GetExtension(const string &path)
{
	string ext;
	size_t pos1 = path.rfind('.');
	if(pos1 != string::npos){
		ext = path.substr(pos1+1, path.size()-pos1);
		string::iterator itr = ext.begin();
		while(itr != ext.end()){
			*itr = tolower(*itr);
			itr++;
		}
		itr = ext.end()-1;
		while(itr != ext.begin()){	// �p�X�̍Ō��\0��X�y�[�X���������Ƃ��̑΍�
			if(*itr == 0 || *itr == 32){
				ext.erase(itr--);
			}
			else{
				itr--;
			}
		}
	}
 
	return ext;
}

/*!
 * �f�B���N�g���쐬(���K�w�Ή�)
 * @param[in] dir �쐬�f�B���N�g���p�X
 * @return ������1,���s��0 (�f�B���N�g�������łɂ���ꍇ��1��Ԃ�)
 */
static int MkDir(string dir)
{
	if(_mkdir(dir.c_str()) != 0){
		char cur_dir[512];
		_getcwd(cur_dir, 512);	// �J�����g�t�H���_���m�ۂ��Ă���
		if(_chdir(dir.c_str()) == 0){	// chdir�Ńt�H���_���݃`�F�b�N
			cout << "MkDir : " << dir << " is already exist." << endl;
			_chdir(cur_dir);	// �J�����g�t�H���_�����ɖ߂�
			return 1;
		}
		else{
			size_t pos = dir.find_last_of("\\/");
			if(pos != string::npos){	// ���K�w�̉\���L��
				int parent = MkDir(dir.substr(0, pos+1));	// �e�f�B���N�g�����ċA�I�ɍ쐬
				if(parent){
					if(_mkdir(dir.c_str()) == 0){
						return 1;
					}
					else{
						return 0;
					}
				}
			}
			else{
				return 0;
			}
		}
	}

	return 1;
}




//-----------------------------------------------------------------------------
// OpenGL
//-----------------------------------------------------------------------------
/*!
 * ���݂̉�ʕ`����摜�t�@�C���Ƃ��ĕۑ�
 * @param[in] fn �t�@�C���p�X
 */
static void SaveDisplay(const string &fn, int w_, int h_)
{
	int c_ = 4;
	unsigned char* data = new unsigned char[w_*h_*c_];

	glReadPixels(0, 0, w_, h_, GL_RGBA, GL_UNSIGNED_BYTE, data);

	// �㉺���]
	int stride = w_*c_;
	for(int j = 0; j < h_/2; ++j){
		for(int i = 0; i < stride; ++i){
			unsigned char tmp = data[j*stride+i];
			data[j*stride+i] = data[(h_-j-1)*stride+i];
			data[(h_-j-1)*stride+i] = tmp;
		}
	}

	string ext = GetExtension(fn);
	if(ext == "bmp"){
		WriteBitmapFile(fn, data, w_, h_, c_, RX_BMP_WINDOWS_V3);
		cout << "saved the screen image to " << fn << endl;
	}
	else if(ext == "png"){
		WritePngFile(fn, data, w_, h_, c_);
		cout << "saved the screen image to " << fn << endl;
	}

	delete [] data;
}

/*!
 * xyz���`��(x��:��,y��:��,z��:��)
 * @param[in] len ���̒���
 */
inline int DrawAxis(double len, double line_width = 5.0)
{
	glLineWidth(line_width);

	// x��
	glColor3f(1.0, 0.0, 0.0);
	glBegin(GL_LINES);
	glVertex3d(0.0, 0.0, 0.0);
	glVertex3d(len, 0.0, 0.0);
	glEnd();

	// y��
	glColor3f(0.0, 1.0, 0.0);
	glBegin(GL_LINES);
	glVertex3d(0.0, 0.0, 0.0);
	glVertex3d(0.0, len, 0.0);
	glEnd();

	// z��
	glColor3f(0.0, 0.0, 1.0);
	glBegin(GL_LINES);
	glVertex3d(0.0, 0.0, 0.0);
	glVertex3d(0.0, 0.0, len);
	glEnd();

	return 1;
}

/*!
 * �~���`��
 * @param[in] rad,len ���a�ƒ��S����������
 * @param[in] slices  �|���S���ߎ�����ۂ̕�����
 */
static void DrawCylinder(double rad, double len, int up, int slices)
{
	GLUquadricObj *qobj;
	qobj = gluNewQuadric();

	glPushMatrix();
	switch(up){
	case 0:
		glRotatef(-90.0, 0.0, 1.0, 0.0);
		glTranslatef(0.0, 0.0, -0.5*len);
		break;
	case 1:
		glRotatef(-90.0, 1.0, 0.0, 0.0);
		glTranslatef(0.0, 0.0, -0.5*len);
		break;
	case 2:
		glTranslatef(0.0, 0.0, -0.5*len);
		break;
	default:
		glTranslatef(0.0, 0.0, -0.5*len);
	}

	gluQuadricDrawStyle(qobj, GLU_FILL);
	gluQuadricNormals(qobj, GLU_SMOOTH);
	gluCylinder(qobj, rad, rad, len, slices, slices);

	glPushMatrix();
	glRotatef(180.0, 1.0, 0.0, 0.0);
	gluDisk(qobj, 0.0, rad, slices, slices);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0, 0.0, len);
	gluDisk(qobj, 0.0, rad, slices, slices);
	glPopMatrix();

	glPopMatrix();
}

/*!
 * �J�v�Z���`��(�~���̗��[�ɔ����������`)
 * @param[in] rad,len ���a�ƒ��S����������
 * @param[in] slices  �|���S���ߎ�����ۂ̕�����
 */
static void DrawCapsule(double rad, double len, int up, int slices)
{
	GLUquadricObj *qobj;
	qobj = gluNewQuadric();

	glPushMatrix();
	switch(up){
	case 0:
		glRotatef(-90.0, 0.0, 1.0, 0.0);
		glTranslatef(0.0, 0.0, -0.5*len);
		break;
	case 1:
		glRotatef(-90.0, 1.0, 0.0, 0.0);
		glTranslatef(0.0, 0.0, -0.5*len);
		break;
	case 2:
		glTranslatef(0.0, 0.0, -0.5*len);
		break;
	default:
		glTranslatef(0.0, 0.0, -0.5*len);
	}

	gluQuadricDrawStyle(qobj, GLU_FILL);
	gluQuadricNormals(qobj, GLU_SMOOTH);
	gluCylinder(qobj, rad, rad, len, slices, slices);

	glPushMatrix();
	glutSolidSphere(rad, slices, slices);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0, 0.0, len);
	glutSolidSphere(rad, slices, slices);
	glPopMatrix();

	glPopMatrix();

}







/*!
 * ���C/�����ƎO�p�`�̌���
 * @param[in] P0,P1 ���C/�����̒[�_or���C��̓_
 * @param[in] V0,V1,V2 �O�p�`�̒��_���W
 * @param[out] I ��_���W
 * @retval 1 ��_I�Ō��� 
 * @retval 0 ��_�Ȃ�
 * @retval 2 �O�p�`�̕��ʓ�
 * @retval -1 �O�p�`��"degenerate"�ł���(�ʐς�0�C�܂�C�������_�ɂȂ��Ă���)
 */
static int IntersectSegmentTriangle(Vec3 P0, Vec3 P1,			// Segment
									Vec3 V0, Vec3 V1, Vec3 V2,	// Triangle
									Vec3 &I, Vec3 &n, float rp)			// Intersection point (return)
{
	// �O�p�`�̃G�b�W�x�N�g���Ɩ@��
	Vec3 u = V1-V0;
	Vec3 v = V2-V0;
	n = Unit(cross(u, v));
	if(RXFunc::IsZeroVec(n)){
		return -1;	// �O�p�`��"degenerate"�ł���(�ʐς�0)
	}

	// ����
	Vec3 dir = P1-P0;
	double a = dot(n, P0-V0);
	double b = dot(n, dir);
	if(fabs(b) < 1e-10){	// �����ƎO�p�`���ʂ����s
		if(a == 0){
			return 2;	// ���������ʏ�
		}
		else{
			return 0;	// ��_�Ȃ�
		}
	}

	// ��_�v�Z

	// 2�[�_�����ꂼ��قȂ�ʂɂ��邩�ǂ����𔻒�
	float r = -a/b;
	Vec3 offset = Vec3(0.0);
	float dn = 0;
	float sign_n = 1;
	if(a < 0){
		return 0;
	}

	if(r < 0.0){
		return 0;
	}
	else{
		if(fabs(a) > fabs(b)){
			return 0;
		}
		else{
			if(b > 0){
				return 0;
			}
		}
	}

	// �����ƕ��ʂ̌�_
	I = P0+r*dir;//+offset;

	// ��_���O�p�`���ɂ��邩�ǂ����̔���
	double uu, uv, vv, wu, wv, D;
	uu = dot(u, u);
	uv = dot(u, v);
	vv = dot(v, v);
	Vec3 w = I-V0;
	wu = dot(w, u);
	wv = dot(w, v);
	D = uv*uv-uu*vv;

	double s, t;
	s = (uv*wv-vv*wu)/D;
	if(s < 0.0 || s > 1.0){
		return 0;
	}
	
	t = (uv*wu-uu*wv)/D;
	if(t < 0.0 || (s+t) > 1.0){
		return 0;
	}

	return 1;
}



#endif // #ifndef _RX_UTILS_H_