/*! 
 @file kite.h

 @brief Kite Simulation
 
 @author Taichi Okamoto
 @date 2008
*/

#ifndef _KITE
#define _KITE


#include <vector>

#include "rx_utility.h"		// Vector classes
#include "rx_matrix.h"		// Matrix classes
#include "rx_quaternion.h"	// Quaternion classes

#include "rx_solver.h"

using namespace std;

//-----------------------------------------------------------------------------------
// �}�N��
//-----------------------------------------------------------------------------------
//���֘A
//�����͐�΂Ɋ�Œ�`���邱��
//(���݂̎d�l�ł͋����ł͔j���񂷂�)
#define COL_NUM 13							//���̉����������_��->(4�̔{��+1)
#define ROW_NUM 13							//���̏c���������_��

#define RHO 1.21							//��C���x(1.2kg/m^3)
#define G_ACCE -9.8							//�d�͉����x

#define KITE_RHO 0.2						//���̖��x? ��{�I�ɂ͈����₷���悤200g/1m^2

//�e�[�u���֌W
#define TABLE_S 0.68						//�A�X�y�N�g��
#define TABLE_L 1.48						//�A�X�y�N�g��

//�����֘A
#define LINE_SP 20							//�����̕�����
#define LINE_K 10.0*((double)LINE_SP)		//�΂˒萔
#define LINE_D 1.0							//�����W��
#define LINE_RHO 0.2						//���ʖ��x
#define LINE_E 0.02							//��R�W��

//�����ۊ֘A
#define TAIL_NUM 2							//�����ۂ̐�

#define TAIL_SP 10							//�����ۂ̕�����
#define TAIL_K 10.0*((double)TAIL_SP)		//�΂˒萔
#define TAIL_D 1.0							//�����W��
#define TAIL_RHO 0.2						//���ʖ��x
#define TAIL_E 0.02							//��R�W��

//����݊֘A
#define BAR_SP (COL_NUM-1)					//���̕�����
#define BAR_L 1.0							//���̒���
#define BAR_WIDTH 0.016						//���f�ʂ̕�(�����`�f�ʂ̏ꍇ,���������˂�)
											//(���悻0.02m�قǂ���͂�����ω����킩��)
#define INV_BAMBOO_E pow(10.0,-9.0)			//�|�̃����O���̋t��


//---------------------------------------------------------------------------------------------------------------------
//���V�~�����[�^
//---------------------------------------------------------------------------------------------------------------------
namespace kite3d
{
	//! structure "quadrangle"
	struct quadrangle
	{
		int index[4];			//�l�p�`�v�f���\������3�_�̃C���f�b�N�X
		double S;				//�l�p�`�v�f�̖ʐ�
		double mass;			//�l�p�`�v�f�̎���
		Vec3 cg;				//�l�p�`�v�f�̏d�S(���[�J���n�ɂ�������W)
		Vec3 normal;			//�l�p�`�v�f�̖ʖ@��
		Vec3 local_inertia_mom;	//�l�p�`�̃��[�J���������[�����g
		Vec3 local_inertia_pro;	//�l�p�`�̃��[�J���������[�����g

		//���̐݌v
		double b;				//�����`�̕�
		double c;				//�����`�̍���

	};

	//! structure "kite_tail"
	struct kite_tail
	{
		int set_point;			//�����\������_�Q�̒��ŐK���Ɛڑ�����Ă���_�ԍ�
		double l;				//�K���̒���
		vector<Vec3> pos;		//���_�ʒu
		vector<Vec3> pos_pre;	//1step�O�̎��_�ʒu
		vector<Vec3> vel;		//���_���x
		vector<Vec3> vel_pre;	//1step�O�̎��_���x
		vector<Vec3> frc;		//���_�ɉ���鍇��

	};

	//! structure "kite_3d"
	//(���ׂĂ̕ϐ��𗘗p���Ă���Ƃ͌���Ȃ�)
	struct kite_3d
	{
		//initialize--------------------------------
		double l_init;				//���̏�������
		double l_now;				//���݂̎��̒���
		double l_check;				//�[�_�Ԃ̋���
		vector<Vec3> line_pos;		//�����\�����鎿�_�̈ʒu
		vector<Vec3> line_pos_pre;	//�����\�����鎿�_��1step�O�̈ʒu
		vector<Vec3> line_vel;		//�����\�����鎿�_�̑��x
		vector<Vec3> line_vel_pre;	//�����\�����鎿�_��1step�O�̑��x
		vector<Vec3> line_frc;		//�����\�����鎿�_�ɉ���鍇��

		//model-------------------------------------
		//���̐݌v
		double b;					//���S�̂̍ő啝
		double c;					//���S�̂̍ő卂��
		double AR;					//�A�X�y�N�g��

		double mass;				//���S�̂̎���
		double S;					//���S�̖̂ʐ�

		int p_num;					//�����\������_�̐�
		//���[�J�����W(�l�͈��C���W�̋N�_�͎��ڒ��S)
		vector<Vec3> local_cd;		//�����\������_�̍��W(���[�J���n)
		//�O���[�o�����W(�����_�����O���ɗp����)
		vector<Vec3> global_cd;		//�����\������_�̍��W(�O���[�o���n)

		vector<Vec3> design_cd;		//���_���W(�f�U�C���p���W)
		vector<Vec3> tex_cd;		//���_���W(�e�N�X�`�����W)

		vector<quadrangle> element;	//�l�p�`�v�f
		int q_num;					//�l�p�`�v�f��

		Vec3 s;						//���ڒ��S(�ŏI�I�ɂ̓��[�J�����W�Ŋi�[)
		Vec3 cg;					//�d�S���W(���[�J�����W�ł͂Ȃ�->���[�J�����W�ł͌��_�ɂ�����)

		rxMatrix3 inertia;			//�����e���\��
		rxMatrix3 inertia_inv;		//�����e���\���̋t�s��

		//sim---------------------------------------
		Vec3 pos;					//�O���[�o���n�ɂ�������̏d�S�ʒu
		Vec3 pos_pre;				//�O���[�o���n�ɂ�����1step�O�̑��̏d�S�ʒu

		Vec3 glb_s_pos;				//�O���[�o���n�ɂ�������̎��ڈʒu

		Vec3 omega;					//�p���x

		rxQuaternion orientation;	//����4����

		Vec3 global_vel;			//�O���[�o���n�ɂ�����ړ����x
		Vec3 local_vel;				//���[�J���n�ɂ���������󂯂鑬�x

		Vec3 euler_angles;			//�I�C���[�p(���̎p��)

		//loads-------------------------------------
		Vec3 frc;					//�׏d
		Vec3 frc_pre;				//�׏d
		Vec3 mom;					//���[�����g
		Vec3 T_spring[2];			//�΂˂ɂ�������

	};


	//������
	void initialize_sim(void);		//���p�����[�^�̏�����
	void initialize_options(void);	//�I�v�V�����̏�����
	void create_model_rec(void);		//���f���̍쐬
	void create_model_yak(void);		//���f���̍쐬
	void create_model_dia(void);		//���f���̍쐬

	//�X�e�b�v��i�߂�
	void step_simulation(double dt);

	//��
	void set_wind(double dt);

	//�׏d�ƃ��[�����g�̌v�Z
	void calc_loads(double dt);

	//�|�W�V�����̌v�Z
	void calc_line_pos(double dt);//��(���̎��R�[�����̈ʒu�ɑΉ�)
	void calc_tail_pos(int n,double dt);//������

	//�`��֌W
	void draw_kite(void);	//���{��
	void draw_line(void);	//����
	void draw_tail(void);	//������
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

	//���[�U�C���^�t�F�[�X(�n�v�e�B�b�N�f�o�C�X)�ɂ���
	Vec3 calc_UI_force(void);

	//����݊֌W
	void initialize_deflection(void);	//������
	void calc_deflection(double P);		//����݂̌v�Z
	void keep_long_deflection(void);		//������ۂ�

	double calc_ex_Spring(double T,double dt);
}



#endif