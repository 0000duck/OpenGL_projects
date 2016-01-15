/*
	a class to make a simple kite simulation system
	date: 2015
*/

#ifndef KITESIMULATION_H
#define KITESIMULATION_H

#include <vector>

#include "rx_utility.h"			//Vector classes
#include "rx_matrix.h"			//Matrix classes
#include "rx_quaternion.h"		//Quaternion classes

#include "string_simulation.h"
#include "fluid_solver.h"
using namespace std;


#define DAMP_yaw 0.2
#define DAMP 0.0
//MACRO
#define COL_NUM 13							//���̉����������_��->(4�̔{��+1)
#define ROW_NUM 13							//���̏c���������_��

#define RHO 1.21							//air density(1.2kg/m^3)
#define G_ACCE -9.8							//gravity

#define KITE_RHO 0.2						//���̖��x? ��{�I�ɂ͈����₷���悤200g/1m^2
//�e�[�u���֌W
#define TABLE_S 0.68						//�A�X�y�N�g��
#define TABLE_L 1.48						//�A�X�y�N�g��

//deflection
#define BAR_SP (COL_NUM-1)					//���̕�����
#define BAR_L 1.0							//���̒���
#define BAR_WIDTH 0.016						//���f�ʂ̕�(�����`�f�ʂ̏ꍇ,���������˂�)
											//(���悻0.02m�قǂ���͂�����ω����킩��)
#define INV_BAMBOO_E pow(10.0,-9.0)			//�|�̃����O���̋t��
//quadrangle shape
class KiteSimulation
{
	struct quadrangle
	{
		int index[4];				//index of the 4 point of the quadrangle
		double S;					//surface area
		double mass;				//mass of the quadrangle
		Vec3 cg;					//center of gravity
		Vec3 normal;				//normal of the kite
		Vec3 local_inertia_mom;		//local moment of inertia of the quadrangle
		Vec3 local_inertia_pro;		//local moment of inertia of the quadrangle

		//kite design
		double b;					//width of the quadrangle
		double c;					//height of the quadrangle
	};
protected:

	double l_init;					//init length of string
	//Vec3 line_startPos;
	//Vec3 line_endPos;
	//kite model
	//kite design
	double k_b;						//width of the kite
	double k_c;						//height of the kite
	double k_AR;						//aspect ratio

	//double per_line_mass;
	double k_mass;					//mass of kite
	double k_S;						//surface area of kite
	Vec3 Wind;
	//Vec3 wind_vel;
	int k_p_num;						//�����\������_�̐�
	//���[�J�����W(�l�͈��C���W�̋N�_�͎��ڒ��S)
	vector<Vec3> k_local_cd;		//�����\������_�̍��W(���[�J���n)
	//�O���[�o�����W(�����_�����O���ɗp����)
	vector<Vec3> k_global_cd;		//�����\������_�̍��W(�O���[�o���n)

	vector<Vec3> k_design_cd;			//poins' coordinates(design) 
	vector<Vec3> k_tex_cd;			//points' coordinates(texture)

	vector<quadrangle> element;		//quadrangle element
	int k_q_num;						//number of the quadrangle element

	Vec3 k_s;						//���ڒ��S(�ŏI�I�ɂ̓��[�J�����W�Ŋi�[)
	Vec3 k_cg;					//�d�S���W(���[�J�����W�ł͂Ȃ�->���[�J�����W�ł͌��_�ɂ�����)

	rxMatrix3 inertia;			//inertia
	rxMatrix3 inertia_inv;		//inverse of the inertia

	//sim
	Vec3 k_pos;					//�O���[�o���n�ɂ�������̏d�S�ʒu
	Vec3 k_pos_pre;				//�O���[�o���n�ɂ�����1step�O�̑��̏d�S�ʒu

	Vec3 k_glb_s_pos;				//�O���[�o���n�ɂ�������̎��ڈʒu

	Vec3 k_omega;					//�p���x

	rxQuaternion k_orientation;	//����4����

	Vec3 k_global_vel;			//�O���[�o���n�ɂ�����ړ����x
	Vec3 k_local_vel;				//���[�J���n�ɂ���������󂯂鑬�x

	Vec3 k_euler_angles;			//�I�C���[�p(���̎p��)

	//loads-------------------------------------
	Vec3 k_frc;					//�׏d
	Vec3 k_frc_pre;				//�׏d
	Vec3 k_mom;					//���[�����g
	Vec3 k_T_spring[2];			//�΂˂ɂ�������

public:
	//init
	void initKite(void);		//init the para of kite
	void create_model_rec(void);		//create the model
	void create_model_yak(void);		
	void create_model_dia(void);		
	//void setPerVexMass(double m)	{this->per_line_mass = m;}
	//void setLineStartPoint(Vec3 startpoint) { this->line_startPos = startpoint; }

	//step update
	void step_simulation(double dt);

	//set the length of the string
	void set_length(double length){		l_init = length;	}
	double get_length()	{	return this->l_init;	}
	//set the wind
	void set_wind(double dt);
	//Vec3 get_wind() {return this->wind_vel;}

	//�׏d�ƃ��[�����g�̌v�Z
	void calc_loads(double dt);

	//calc the position
	void calc_line_pos(double dt);//��(���̎��R�[�����̈ʒu�ɑΉ�)

	//draw
	void draw_kite(void);	//draw the kite

	void draw_options_01(void);	//�v���O�����m�F�p�I�v�V����
	void draw_options_02(void);	//�v���O�����m�F�p�I�v�V����
	void draw_options_03(void);	//�v���O�����m�F�p�I�v�V����

	//�t�@�C���ǂݍ���
	int  read_file(const string &file_name, vector<double> &datas);
	void read_file(const string &file_name);
	void read_table(void);					//.dat����e�[�u����ǂݍ���

	//�e�[�u���̎Q��
	double search_alpha_CD(double alpha,double AR);
	double search_alpha_CL(double alpha,double AR);
	double search_alpha_x(double alpha);		//��-x�e�[�u��(�}���p���畗�S�����߂�)

	//�����_�����O�p���W�擾
	void calc_render_pos(void);

	//calc the force from user
	Vec3 calc_UI_force(void);

	//deflection
	void initialize_deflection(void);	//init the deflection
	void calc_deflection(double P);		//calculate the deflection 
	void keep_long_deflection(void);		//������ۂ�

	//double calc_ex_Spring(double T,double dt);
};

/*!
 * Heron�̌���
 * @note		3���_�̍��W����O�p�`�ʐς����߂�
 * @param[in]	A �O�p�`���_(Vec3�^)
 * @param[in]	B �O�p�`���_(Vec3�^)
 * @param[in]	C �O�p�`���_(Vec3�^)
 * @return		�O�p�`�ʐς�Ԃ�(double�^)
 */
inline double 
Heron(Vec3 A, Vec3 B, Vec3 C)
{
	Vec3 BA,CB,AC;	//��
	double a,b,c;	//�O�ӂ̒���
	double s,r;
	double TS;		//�O�p�`�̖ʐ�

	BA=B-A;
	CB=C-B;
	AC=A-C;
	//�ӂ̒���
	a=norm(BA);	//A��B�̕ӂ̒���
	b=norm(CB);	//B��C�̕ӂ̒���
	c=norm(AC);	//C��A�̕ӂ̒���

	s=0.5*(a+b+c);
	r=s*(s-a)*(s-b)*(s-c);	//�������̒��g
	TS=sqrt(r);				//�ʐ�

	return TS;
}

/*!
 * vector�R���e�i���m�̓��όv�Z
 * @param 
 * @return 
 */
inline double 
DotProductV(double a[], double b[], int num)
{
	double d = 0.0;
	for(int i = 0; i < num; ++i){
		d += a[i]*b[i];
	}
	return d;
}
/*!
 * �������z(CG)�\���o
 * @param[in] func y = AP���v�Z����֐��I�u�W�F�N�g
 * @param[in] b �E�Ӎ�
 * @param[out] x �����i�[����
 * @param[inout] max_iter �ő唽����
 * @param[inout] tol ���e�덷
 */
inline int 
cgSolverMatrix(int num, double A[], 
					      double b[], double x[], int &max_iter, double &tol)
{
	int k, idx;
	int size = num;
	double tol2 = tol*tol;

#define LARGE_NUM 100 //�\���ȑ傫�����m�ۗp

	double r[LARGE_NUM]={0},p[LARGE_NUM]={0},y[LARGE_NUM]={0};

	// ��0�ߎ����ɑ΂���c���̌v�Z
	for(idx = 0; idx < size ; ++idx){
		x[idx] = 0.0;
		r[idx] = b[idx];
		p[idx] = b[idx];
	}

	double resid;
	double norm2_b = DotProductV(b, b, num);
	if(fabs(norm2_b) < RX_FEQ_EPS) norm2_b = 1.0;
		
	double rr0 =DotProductV(r, r, num), rr1;
	if((resid = rr0/norm2_b) <= tol2){
		tol = sqrt(resid);
		max_iter = 0;

		return 0;
	}

	double alpha0, beta0;
	for(k = 0; k < max_iter; k++){
		// y = Ap �̌v�Z
		for(int i = 0; i < size; ++i){
			y[i] = 0.0;
			for(int j = 0; j < size; j++){
				y[i] += A[num*i+j]*p[j];
			}
		}

		// alpha = r*r/(P*AP)�̌v�Z
		alpha0 = rr0/DotProductV(p, y, num);

		// ��x�A�c��r�̍X�V
		for(idx = 0; idx < size ; ++idx){
			x[idx] += alpha0*p[idx];
			r[idx] -= alpha0*y[idx];
		}

		// r_(k+1)*r_(k+1)
		rr1 = DotProductV(r, r, num);

		if((resid = rr1/norm2_b) <= tol2){
			tol = sqrt(resid);
			max_iter = k+1;

			return 0;     
		}

		beta0 = rr1/rr0;
		for(idx = 0; idx < size ; ++idx){
			p[idx] = r[idx]+beta0*p[idx];
		}

		rr0 = rr1;
	}

	tol = resid;

	return k;
}

#endif //KITESIMULATION_H