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
// OpenGL
//-----------------------------------------------------------------------------
/*!
 * ������`��
 * @param[in] static_str ������
 * @param[in] w,h �E�B���h�E�T�C�Y
 */
void DrawStrings(vector<string> &static_str, int w, int h)
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