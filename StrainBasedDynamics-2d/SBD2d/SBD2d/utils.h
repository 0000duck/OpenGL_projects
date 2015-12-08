/*! @file utils.h
	
	@brief �l�X�Ȋ֐�
 
	@author Makoto Fujisawa
	@date 2012
*/

#ifndef _RX_UTILS_H_
#define _RX_UTILS_H_

//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
// STL
#include <vector>
#include <string>

// OpenGL
#include <GL/glew.h>
#include <GL/glut.h>

// Vec3��l�X�Ȋ֐�
#include "rx_utility.h"	

using namespace std;

//-----------------------------------------------------------------------------
// �萔�E��`
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// ���l�v�Z
//-----------------------------------------------------------------------------
/*!
 * 2x2�s��̍s�񎮂̌v�Z
 *  | m[0] m[1] |
 *  | m[2] m[3] |
 * @param[in] m ���̍s��
 * @return �s�񎮂̒l
 */
static inline double CalDetMat2x2(const double *m)
{
    return m[0]*m[3]-m[1]*m[2];
}
 
/*!
 * 2x2�s��̋t�s��̌v�Z
 *  | m[0] m[1] |
 *  | m[2] m[3] |
 * @param[in] m ���̍s��
 * @param[out] invm �t�s��
 * @return �t�s��̑���
 */
static inline bool CalInvMat2x2(const double *m, double *invm)
{
    double det = CalDetMat2x2(m);
    if(fabs(det) < RX_FEQ_EPS){
        return false;
    }
    else{
        double inv_det = 1.0/det;
        invm[0] =  inv_det*m[3];
        invm[1] = -inv_det*m[1];
        invm[2] = -inv_det*m[2];
        invm[3] =  inv_det*m[0];
        return true;
    }
}

/*!
 * 3x3�s��̍s�񎮂̌v�Z
 *  | m[0] m[1] m[2] |
 *  | m[3] m[4] m[5] |
 *  | m[6] m[7] m[8] |
 * @param[in] m ���̍s��
 * @return �s�񎮂̒l
 */
static inline double CalDetMat3x3(const double *m)
{
    // a11a22a33+a21a32a13+a31a12a23-a11a32a23-a31a22a13-a21a12a33
    return m[0]*m[4]*m[8]+m[3]*m[7]*m[2]+m[6]*m[1]*m[5]
          -m[0]*m[7]*m[5]-m[6]*m[4]*m[2]-m[3]*m[1]*m[8];
}
 
/*!
 * 3x3�s��̋t�s��̌v�Z
 *  | m[0] m[1] m[2] |
 *  | m[3] m[4] m[5] |
 *  | m[6] m[7] m[8] |
 * @param[in] m ���̍s��
 * @param[out] invm �t�s��
 * @return �t�s��̑���
 */
static inline bool CalInvMat3x3(const double *m, double *invm)
{
    double det = CalDetMat3x3(m);
    if(fabs(det) < RX_FEQ_EPS){
        return false;
    }
    else{
        double inv_det = 1.0/det;
 
        invm[0] = inv_det*(m[4]*m[8]-m[5]*m[7]);
        invm[1] = inv_det*(m[2]*m[7]-m[1]*m[8]);
        invm[2] = inv_det*(m[1]*m[5]-m[2]*m[4]);
 
        invm[3] = inv_det*(m[5]*m[6]-m[3]*m[8]);
        invm[4] = inv_det*(m[0]*m[8]-m[2]*m[6]);
        invm[5] = inv_det*(m[2]*m[3]-m[0]*m[5]);
 
        invm[6] = inv_det*(m[3]*m[7]-m[4]*m[6]);
        invm[7] = inv_det*(m[1]*m[6]-m[0]*m[7]);
        invm[8] = inv_det*(m[0]*m[4]-m[1]*m[3]);
 
        return true;
    }
}


//-----------------------------------------------------------------------------
// OpenGL
//-----------------------------------------------------------------------------
/*!
 * ������`��
 * @param[in] static_str ������
 * @param[in] w,h �E�B���h�E�T�C�Y
 */
static void DrawStrings(vector<string> &static_str, int w, int h)
{
	glDisable(GL_LIGHTING);
	// ���s���e�ɂ���
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, w, 0, h);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
 
	float x0 = 5.0f;
	float y0 = h-20.0f;
 
	// ��ʏ㕔�Ƀe�L�X�g�`��
	for(int j = 0; j < (int)static_str.size(); ++j){
		glRasterPos2f(x0, y0);
 
		int size = (int)static_str[j].size();
		for(int i = 0; i < size; ++i){
			char ic = static_str[j][i];
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, ic);
		}
 
		y0 -= 20;
	}
 
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}



#endif // #ifndef _RX_UTILS_H_