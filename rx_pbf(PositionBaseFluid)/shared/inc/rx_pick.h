/*! @file rx_pick.h
	
	@brief OpenGL�̃Z���N�V�������[�h���g�������_�s�b�N
		   ��`�I���ɂ��Ή�
 
	@author Makoto Fujisawa
	@date   2008-06, 2010-03, 2013-02
*/

#pragma warning (disable: 4244)

#ifndef _RX_PICK_H_
#define _RX_PICK_H_


//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
#include <cstdio>
#include <cstdlib>

#include <vector>
#include <algorithm>

#include <GL/glut.h>

#include "rx_utility.h"


//-----------------------------------------------------------------------------
// ��`
//-----------------------------------------------------------------------------
using namespace std;

//! �s�b�N���ꂽ�I�u�W�F�N�g�Ɋւ�����
struct rxPickInfo
{
	GLuint name;		//!< �q�b�g�����I�u�W�F�N�g�̖��O
	float min_depth;	//!< �v���~�e�B�u�̃f�v�X�l�̍ŏ��l
	float max_depth;	//!< �v���~�e�B�u�̃f�v�X�l�̍ő�l
};

static inline bool CompFuncPickInfo(rxPickInfo a, rxPickInfo b)
{
	return a.min_depth < b.min_depth;
}

//-----------------------------------------------------------------------------
// OpenGL�ɂ��I�u�W�F�N�g�s�b�N
//-----------------------------------------------------------------------------
class rxGLPick
{
	void (*m_fpDraw)(void*);	//!< �s�b�N�p�`��֐�
	void (*m_fpProj)(void*);	//!< ���e�ϊ��֐�
	void *m_pDrawFuncPtr;		//!< �`��֐����Ăяo�����߂̃|�C���^(�����o�֐��|�C���^�p)
	void *m_pProjFuncPtr;		//!< ���e�ϊ��֐����Ăяo�����߂̃|�C���^(�����o�֐��|�C���^�p)
 
	int m_iSelBufferSize;		//!< �Z���N�V�����o�b�t�@�̃T�C�Y
	GLuint* m_pSelBuffer;		//!< �Z���N�V�����o�b�t�@

	int m_iLastPick;			//!< �Ō�Ƀs�b�N���ꂽ�I�u�W�F�N�g�̔ԍ�
 
public:
	rxGLPick()
	{
		m_iSelBufferSize = 4096;
		m_pSelBuffer = new GLuint[m_iSelBufferSize];
	}
	~rxGLPick()
	{
		if(m_pSelBuffer) delete [] m_pSelBuffer;
	}
 
	void Set(void (*draw)(void*), void* draw_ptr, void (*proj)(void*), void* proj_ptr)
	{
		m_fpDraw = draw;
		m_fpProj = proj;
		m_pDrawFuncPtr = draw_ptr;
		m_pProjFuncPtr = proj_ptr;
	}
 
protected:
	/*!
	 * OpenGL�ɂ��s�b�N�Ńq�b�g�������̂���ŏ��̃f�v�X�l�������̂�I������
	 * @param hits �q�b�g��
	 * @param buf  �I���o�b�t�@
	 * @return �q�b�g�����I�u�W�F�N�g�ԍ�
	 */
	vector<rxPickInfo> selectHits(GLint nhits, GLuint buf[])
	{
		vector<rxPickInfo> hits;
 
		float depth_min = 100.0f;
		float depth1 = 1.0f;
		float depth2 = 1.0f;
 
		GLuint depth_name;
		GLuint *ptr;
		
		// �q�b�g�����f�[�^�Ȃ�
		if(nhits <= 0) return hits;
		
		hits.resize(nhits);

		// �|�C���^����Ɨpptr�֓n���D
		ptr = (GLuint*)buf;
		for(int i = 0; i < nhits; ++i){
			// �q�b�g�����I�u�W�F�N�g�̖��O
			depth_name = *ptr;
			ptr++;

			// �q�b�g�����v���~�e�B�u�̃f�v�X�l�̍ŏ��l
			depth1 = (float) *ptr/0x7fffffff;
			ptr++;

			// �q�b�g�����v���~�e�B�u�̃f�v�X�l�̍ő�
			depth2 = (float) *ptr/0x7fffffff;
			ptr++;

			hits[i].name = (int)(*ptr);
			hits[i].min_depth = depth1;
			hits[i].max_depth = depth2;

			ptr++;
		}

		return hits;
	}
 
	/*!
	 * �}�E�X�I��
	 * @param[in] x,y �I�𒆐S���W(�}�E�X���W�n)
	 * @param[in] w,h �I��͈�
	 * @retval int �s�b�N�I�u�W�F�N�g�ԍ�
	 */
	vector<rxPickInfo> pick(int x, int y, int w, int h)
	{
		GLint viewport[4];
 
		glGetIntegerv(GL_VIEWPORT, viewport);
		glSelectBuffer(m_iSelBufferSize, m_pSelBuffer);
 
		glRenderMode(GL_SELECT);
 
		glInitNames();
		glPushName(0);
 
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
 
		gluPickMatrix(x, viewport[3]-y, w, h, viewport);
		m_fpProj(m_pProjFuncPtr);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
 
		m_fpDraw(m_pDrawFuncPtr);
		glLoadName(0);
 
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glPopName();
 
		GLint nhits;
		nhits = glRenderMode(GL_RENDER);
		
		vector<rxPickInfo> hits = selectHits(nhits, m_pSelBuffer);
		glMatrixMode(GL_MODELVIEW);

		if(nhits > 0) m_iLastPick = hits[0].name;
		return hits;
	}
 
 
public:
	/*!
	 * �I�u�W�F�N�g�̃}�E�X�I��
	 * @param[in] x,y �}�E�X���W
	 * @retval true �s�b�N����
	 * @retval false �s�b�N���s
	 */
	int Pick(int x, int y)
	{
		vector<rxPickInfo> hits = pick(x, y, 16, 16);
		if(hits.empty()){
			m_iLastPick = -1;
			return -1;
		}
		else{
			std::sort(hits.begin(), hits.end(), CompFuncPickInfo);
			m_iLastPick = hits[0].name;
			return hits[0].name;
		}
 
		return -1;
	}

	/*!
	 * �I�u�W�F�N�g�̃}�E�X�I��(��`�͈͎w��, �����I��)
	 * @param[in] sx,sy �n�_
	 * @param[in] ex,ey �I�_
	 * @retval true �s�b�N����
	 * @retval false �s�b�N���s
	 */
	vector<rxPickInfo> Pick(int sx, int sy, int ex, int ey)
	{
		int x, y, w, h;

		if(ex < sx) RX_SWAP(ex, sx);
		if(ey < sy) RX_SWAP(ey, sy);

		x = (ex+sx)/2;
		y = (ey+sy)/2;
		w = ex-sx;
		h = ey-sy;

		return pick(x, y, w, h);
	}

	/*!
	 * �Ō�Ƀs�b�N���ꂽ�I�u�W�F�N�g�̔ԍ���Ԃ�
	 * @return �s�b�N���ꂽ�I�u�W�F�N�g�̔ԍ�
	 */
	int GetLastPick(void) const
	{
		return m_iLastPick;
	}
};


/*!
 * �}�E�X�h���b�O�̈�̕`��(��`�ƒ���)
 * @param[in] type 1�Œ���,2�ŋ�`
 * @param[in] p0,p1 �}�E�X�h���b�O�J�n�_�C�I���_
 */
static inline void DrawRubber(int type, Vec2 p0, Vec2 p1, int w, int h)
{
	glDisable(GL_LIGHTING);
	glColor3f(0.0, 0.0, 0.0);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, w, h, 0);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	if(type == 1){		// ��`
		glColor3d(1.0, 1.0, 1.0);
		glPushMatrix();
		glBegin(GL_LINE_LOOP);
		glVertex2d(p0[0], p0[1]);
		glVertex2d(p1[0], p0[1]);
		glVertex2d(p1[0], p1[1]);
		glVertex2d(p0[0], p1[1]);
		glEnd();
		glPopMatrix();
	}
	else if(type == 2){	// ����
		glDisable(GL_LIGHTING);
		glColor3d(0.0, 0.0, 0.0);
		glBegin(GL_LINES);
		glVertex2dv(p0.data);
		glVertex2dv(p1.data);
		glEnd();
	}
	
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

}



#endif // #ifndef _RX_PICK_H_