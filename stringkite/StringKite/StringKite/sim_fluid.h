
#include <cmath>

#ifndef SIM_FLUID
#define SIM_FLUID

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


#endif //SIM_FLUID