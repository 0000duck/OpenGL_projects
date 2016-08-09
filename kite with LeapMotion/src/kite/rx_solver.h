/*! 
 @file solver.h

 @brief Fluid solver
 
 @author Taichi Okamoto
 @date 2008
*/

#include <cmath>

#ifndef _SOLVER
#define _SOLVER

//-----------------------------------------------------------------------------------
//�ϐ�
//-----------------------------------------------------------------------------------
extern double *g_u;//!< ���x���x��������
extern double *g_v;//!< ���x���y��������
extern double *g_w;//!< ���x���z��������
extern double *g_u_prev;//!< 1�菇�O�̑��x���x��������
extern double *g_v_prev;//!< 1�菇�O�̑��x���y��������
extern double *g_w_prev;//!< 1�菇�O�̑��x���z��������

extern double *g_dens;//!< ���x��(���ݖ�����)
extern double *g_dens_prev;//!< 1�菇�O�̖��x��(���ݖ�����)
			   
extern double *g_p_prev;//!< 1�菇�O�̈��͏�
			   
			   
//-----------------------------------------------------------------------------------
// �}�N��
//-----------------------------------------------------------------------------------
#define IX(i,j,k) ((i)+(N+2)*(j)+(N+2)*(N+2)*(k))//!< 3�����̊i�[
#define SWAP(x0,x) {double * tmp=x0;x0=x;x=tmp;}//!< �z��̓���ւ�

#define GRID 16			//!< 1�������̃O���b�h������
#define F_FORCE -1.0	//!< �O��
#define STEP 0.01		//!< �^�C���X�e�b�v��
#define VISC 0.0007		//!< ���S���W��
#define DIFF 0.0		//!< �g�U�W��
using namespace std;

//---------------------------------------------------------------------------------------------------------------------
// ���̃V�~�����[�^
//---------------------------------------------------------------------------------------------------------------------
namespace fluid
{
	static int X_wind=0;//!< x�����̊O�͌�(demo)
	static int Z_wind=1;//!< z�����̊O�͌�(demo)

	static int V_field=0;//!< ���x��̎��o��ON/OFF(demo)
	static int D_tex=0;
	static int frc_view=0;

	void free_data ( void );

	//! �������m��
	void allocate_data ( void );
	//! ���x��̏�����
	void clear_data ( void );

	//! ���x��
	void vel_step ( int n, double * u, double * v, double * w, double * u0, double * v0, double * w0, double visc, double dt );

	//! �O�͍�
	void add_source ( int n, double * x, double * s, double dt );
	//! �g�U��
	void diffuse ( int n, int b, double * x, double * x0, double diff, double dt );
	//! �ڗ���
	void advect ( int n, int b, double * d, double * d0, double * u, double * v,double * w, double dt );
	//! Gauss-Seidal�����@
	void lin_solve ( int n, int b, double * x, double * x0, double a, double c );
	//! ���E����
	void set_bnd ( int n, int b, double * x );
	//! ���ʕۑ�
	void project( int n, double * u, double * v, double * w, double * p, double * div );

	//! ���[�U����
	void get_from_UI ( int N, double * d, double * u, double * v, double * w );

	//! OpenGL�`��
	void draw_box(double mag);
	void draw_velocity(double mag, double scale);
}


#endif