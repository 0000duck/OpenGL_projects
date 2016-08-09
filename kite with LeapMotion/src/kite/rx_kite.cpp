/*! 
 @file kite.cpp

 @brief Kite Simulation
 
 @author Taichi Okamoto
 @date 2008
*/



// OpenGL
#include <GL/glew.h>
#include <GL/glut.h>

#include "rx_kite.h"

double CD_068_table[46];	//!< alpha_CD_068.dat�i�[�p�z��
double CD_148_table[46];	//!< alpha_CD_148.dat�i�[�p�z��
double CL_068_table[46];	//!< alpha_CL_068.dat�i�[�p�z��
double CL_148_table[46];	//!< alpha_CL_148.dat�i�[�p�z��
double x_table[71];		//alpha_x.dat�i�[�p�z��

int	nsteps = 0;

kite3d::kite_3d kite;//�����i�[
kite3d::kite_tail tail[TAIL_NUM];//�����ۏ��i�[

//�Е��͗́C�����͈ʒu
Vec3 Lift[2][2];
Vec3 Drag[2][2];
Vec3 T_tail[TAIL_NUM][2];
Vec3 T_string[2];
Vec3 G[2];

float tension_check;
double ex_Sp_l;
double ex_Sp_pos[2];
double ex_Sp_vel[2];

Vec3 Wind;
Vec3 kite_check;
int v_point[4];

Vec3 spring_ce;

//���l���(�s��,�x�N�g��,�X�J��)
double	L_C_strains[LINE_SP];				//��(1)(�ӂ̂Ђ��݂ɑ���)�x�N�g��(��(5)��B�x�N�g��)(m*1)
double	L_C_grad[LINE_SP][3*(LINE_SP+1)];	//��(2)(�ӂ̌X���ɑ���)�s��(m*3n)
double	L_IM_Cg[3*(LINE_SP+1)][LINE_SP];	//Inv_M�~C_grad�̓]�u�s��(3n*m)
double	L_Cg_IM_Cg[LINE_SP*LINE_SP];		//��(5)(Ax=B)��A�s��(m*m)
double	L_Delta_lambda[LINE_SP];			//�Ƀx�N�g��(m*1)
double	L_Delta_x[3*(LINE_SP+1)];			//�������󂯎���x�x�N�g��(3n*1)

double	T_C_strains[TAIL_SP];				//��(1)(�ӂ̂Ђ��݂ɑ���)�x�N�g��(��(5)��B�x�N�g��)(m*1)
double	T_C_grad[TAIL_SP][3*(TAIL_SP+1)];	//��(2)(�ӂ̌X���ɑ���)�s��(m*3n)
double	T_IM_Cg[3*(TAIL_SP+1)][TAIL_SP];	//Inv_M�~C_grad�̓]�u�s��(3n*m)
double	T_Cg_IM_Cg[TAIL_SP*TAIL_SP];		//��(5)(Ax=B)��A�s��(m*m)
double	T_Delta_lambda[TAIL_SP];			//�Ƀx�N�g��(m*1)
double	T_Delta_x[3*(TAIL_SP+1)];			//�������󂯎���x�x�N�g��(3n*1)

//deflection
Vec2 point[(BAR_SP+1)];
double sp_l;		//�_�Ԋu
double Wind_vel;	//�C�����x
double p_l[2];		//���e�ʐ�(����)


//���l�v�Z�p
double Heron(Vec3 A, Vec3 B, Vec3 C);	//�w�����̌���
double DotProductV(double a[], double b[], int num);
int cgSolverMatrix(int num, double A[], double b[], double x[], int &max_iter, double &tol);

using namespace kite3d;


//---------------------------------------------------------------------------------
//���V�~�����[�^
//---------------------------------------------------------------------------------

/*!
 * @note ���p�����[�^�̏�����
 */
void 
kite3d::initialize_sim(void)
{
	kite_check=Vec3();
	tension_check=0.0f;

	ex_Sp_l=1.0;
	ex_Sp_pos[0]=0.0;
	ex_Sp_pos[1]=ex_Sp_l;
	ex_Sp_vel[0]=0.0;
	ex_Sp_vel[1]=0.0;

//�����C���̂����ۂɊւ��Ă�
//initialize_options�ŗv�f�m�ۍς�
	
	//�����Ɋւ���p�����[�^�̏�����
	kite.l_init=8.0;		//���̒���
	kite.l_now=kite.l_init;
	Vec3 Over,Side;
	for(int i=0;i<=LINE_SP;i++)
	{
		Over=Vec3(1.0,0.0,0.2);	//�΂ߏ��
		normalize(Over);
		Over*=kite.l_init*((double)i)/((double)LINE_SP);
		Side=Vec3(kite.l_init*((double)i)/((double)LINE_SP),0.0,0.0);	//�^��

		//kite.line_pos[i]=Side;
		kite.line_pos[i]=Over;
		kite.line_pos_pre[i]=kite.line_pos[i];
		kite.line_vel[i]=Vec3();
		kite.line_vel_pre[i]=kite.line_vel[i];
		kite.line_frc[i]=Vec3();
	}
	kite.l_check=norm((kite.line_pos[LINE_SP]-kite.line_pos[0]));

	//�����ۂ̒���
	tail[0].l=0.8;	
	tail[1].l=0.8;

	//���{�̂̃p�����[�^�̏�����
	kite.pos=kite.line_pos[LINE_SP];	//�����ʒu(�����̐�[)
	kite.pos_pre=kite.pos;				//1step�O�̈ʒu
	kite.global_vel=Vec3();				//�O���[�o���n�ɂ����鑬�x
	kite.local_vel=Vec3();				//���[�J���n�ɂ����鑬�x

	kite.frc=Vec3();			//�׏d
	kite.frc_pre=kite.frc;		//�׏d
	kite.T_spring[0]=Vec3();	//�΂˂ɂ�������
	kite.T_spring[1]=Vec3();	//�΂˂ɂ�������

	//���̉�]�Ɋւ���p�����[�^�̏�����
	kite.omega=Vec3();		//�p���x

	//����4�����̏�����
	//�����p��(�p�x)
	double roll=0.0;
	double pitch=60.0;
	double yow=0.0;
	double rdrd=RX_DEGREES_TO_RADIANS * RX_DEGREES_TO_RADIANS;//degree����rad�ɕϊ����邽��
	Vec3 dummy=Vec3(roll*rdrd,pitch*rdrd,yow*rdrd);//(roll,pitch,yow)

	kite.orientation.SetEulerAngles(dummy);//�����p���̊i�[

	for(int i=0;i<2;i++)
	{
		Lift[0][i]=Vec3();
		Lift[1][i]=Vec3();
		Drag[0][i]=Vec3();
		Drag[1][i]=Vec3();
		T_tail[0][i]=Vec3();
		T_tail[1][i]=Vec3();
		T_string[i]=Vec3();
		G[i]=Vec3();
	}
}

/*!
 * @note �I�v�V����(�����E�K��)�̕��������̗v�f�m��
 */
void 
kite3d::initialize_options(void)
{
	int i=0,j=0;//�J�E���^

	//�����̕����������[�v
	for(i=0;i<=LINE_SP;i++)
	{
		//�v�f�m��
		kite.line_pos.push_back(Vec3());
		kite.line_pos_pre.push_back(Vec3());
		kite.line_vel.push_back(Vec3());
		kite.line_vel_pre.push_back(Vec3());
		kite.line_frc.push_back(Vec3());
	}

	//�K���̕����������[�v
	for(i=0;i<=TAIL_SP;i++)
	{
		//�K���̖{�������[�v
		for(j=0;j<TAIL_NUM;j++)
		{
			tail[j].pos.push_back(Vec3());
			tail[j].pos_pre.push_back(Vec3());
			tail[j].vel.push_back(Vec3());
			tail[j].vel_pre.push_back(Vec3());
			tail[j].frc.push_back(Vec3());
		}
	}
}

/*!
 * @note ����݂̏�����
 */
void 
kite3d::initialize_deflection(void)
{
	int i=0;

	for(i=0;i<=BAR_SP;i++)
	{
		point[i]=Vec2();//0�ŏ�����
		point[i].data[0]=((double)i)*kite.b/((double)BAR_SP);
	}

	sp_l=kite.b/((double)BAR_SP);//�_�Ԋu
	Wind_vel=0.0;//�C�����x�̏�����

	//���e�ʐ�(����)
	p_l[0]=point[BAR_SP].data[0]-point[0].data[0];
	//�ő傽���
	p_l[1]=0.0;
}

/*!
 * @note ����݂̌v�Z
 * @param[in] P ���ɉ����׏d
*/
void 
kite3d::calc_deflection(double P)
{
	p_l[1]=0.0;//�ő傽��݂̏�����

	int i=0,j=0;

	//����݌v�Z
	//�ʒu�����܂Ȃ������̌v�Z
	double coef[2]={0.0,0.0};
	coef[0]=0.25*P;
	coef[1]=1.0/pow(BAR_WIDTH,4.0);
	coef[1]*=INV_BAMBOO_E;

	//�ʒu�����ޕ����̌v�Z
	for(i=0;i<=BAR_SP;i++)
	{
		j=i;
		if((BAR_SP/2)<i)
		{
			j=BAR_SP-i;
		}

		double var=0.0;
		var=3.0*kite.b*kite.b-4.0*point[j].data[0]*point[j].data[0];
		var*=point[j].data[0];

		point[i].data[1]=var*coef[0]*coef[1];

		//�ő傽��݂̃`�F�b�N
		if(point[i].data[1]>p_l[1])
		{
			p_l[1]=point[i].data[1];
		}
	}

	p_l[1]=fabs(p_l[1]);

	kite3d::keep_long_deflection();
}

/*!
 * @note ����݂��������ۂɖ_�̒�����ۂ�
 */
void 
kite3d::keep_long_deflection(void)
{
	int i=0;

	double l_2=sp_l*sp_l;//�_�Ԋu��2��
	double y=0.0,y_2=0.0;
	double x=0.0,x_2=0.0;

	for(i=0;i<BAR_SP;i++)
	{
		//������y�������������߂�
		y=point[i].data[1]-point[i+1].data[1];
		y=fabs(y);
		//������y����������2��
		y_2=y*y;

		//�ϕ���(x��������)�����߂�
		x_2=l_2-y_2;	//(�Ε�^2-y��������^2)
		x=sqrt(x_2);
		x=fabs(x);		//�O�̂��ߐ�Βl�v�Z�ɂ�萳�ɂ���
		
		point[i+1].data[0]=point[i].data[0]+x;
	}

	p_l[0]=point[BAR_SP].data[0]-point[0].data[0];//���e�ʐ�(����)
}

/*!
 * @note ���̃f�U�C�����烂�f���쐬
 */
void 
kite3d::create_model_rec(void)
{
	//�ŏI�I�ɂ͓��͂��ꂽ�f�U�C������
	//�_�Q�ƎO�p�`�|���S�����쐬

	int i=0,j=0;//�J�E���^

//����---------------------------------
	//double b=0.85;//�ő剡��
	//double c=0.8;//�ő�c��
	double b=0.8;//�ő剡��
	double c=1.2;//�ő�c��

	vector<Vec3> point;	//�_�Q
	int p_num=0;		//�_�Q��

	vector<Vec3> tex_p;	//�e�N�X�`�����W

	for(i=0;i<ROW_NUM;i++)//�c����(x����)
	{
		for(j=0;j<COL_NUM;j++)//������(y����)
		{
			point.push_back(Vec3(((double)i)*c/((double)(ROW_NUM-1)),((double)j)*b/((double)(COL_NUM-1)),0.0 ));
			p_num++;//�o�^�_�����Z
			tex_p.push_back(Vec3( -((double)i)/((double)(ROW_NUM-1)),-((double)j)/((double)(COL_NUM-1)),0.0));
		}
	}
	v_point[0]=0;
	v_point[1]=COL_NUM-1;
	v_point[2]=COL_NUM*ROW_NUM-1;
	v_point[3]=COL_NUM*(ROW_NUM-1);

	vector<int> q_index[4];	//�l�p�`���\������4�_�̃C���f�b�N�X
	int q_num=0;			//�l�p�`�̐�

	for(i=0;i<(ROW_NUM-1);i++)
	{
		for(j=0;j<(COL_NUM-1);j++)
		{
			//�l�p�`���i�[���Ă���
			q_index[0].push_back(i*COL_NUM+j);
			q_index[1].push_back(i*COL_NUM+(j+1));
			q_index[2].push_back((i+1)*COL_NUM+(j+1));
			q_index[3].push_back((i+1)*COL_NUM+j);
			q_num++;
		}
	}

	Vec3 itome=Vec3(0.3*c,0.5*b,0.0);//���ڒ��S

	//���͂���v�Z�p���f���쐬
	kite.b=b;
	kite.c=c;
	kite.AR=b/c;

	kite.p_num=p_num;	//���_���i�[
	kite.q_num=q_num;	//�l�p�`�v�f���i�[

	kite.s=itome;		//���ڒ��S��荞��

	for(i=0;i<p_num;i++)
	{
		kite.tex_cd.push_back(tex_p[i]);

		kite.local_cd.push_back(point[i]);	//���[�J�����W�͌�ŏd�S��ɂȂ�

		kite.design_cd.push_back(point[i]-kite.s);//�f�U�C�����W�͎��ڒ��S����ɂ��Ă���
		kite.global_cd.push_back(QVRotate(kite.orientation,kite.design_cd[i]));	//�O���[�o�����W
		kite.global_cd[i]+=kite.pos;
	}
	
	kite.S=0.0; kite.mass=0.0;	//������
	kite.element.resize(q_num);	//�l�p�`�v�f����element�m��
	for(i=0;i<q_num;i++)
	{
		kite.element[i].b=kite.b;
		kite.element[i].c=kite.c;

		//�l�p�`���\������4�_�̃C���f�b�N�X
		kite.element[i].index[0]=(int)q_index[0][i];
		kite.element[i].index[1]=(int)q_index[1][i];
		kite.element[i].index[2]=(int)q_index[2][i];
		kite.element[i].index[3]=(int)q_index[3][i];

		//�w�����̌������l�p�`�̖ʐς��v�Z
		kite.element[i].S=Heron(kite.local_cd[kite.element[i].index[0]],
							kite.local_cd[kite.element[i].index[1]],
							kite.local_cd[kite.element[i].index[2]])
						+ Heron(kite.local_cd[kite.element[i].index[0]],
							kite.local_cd[kite.element[i].index[2]],
							kite.local_cd[kite.element[i].index[3]]);
		//wxLogMessage(_T("S=%lf"),kite.element[i].S);

		//�l�p�`�̎���(�ʐ�*���x)
		kite.element[i].mass=kite.element[i].S*KITE_RHO;

		Vec3 v1,v2;
		double dummy;
		//�l�p�`�̖ʖ@��
		v1=kite.local_cd[kite.element[i].index[1]]-kite.local_cd[kite.element[i].index[0]];
		v2=kite.local_cd[kite.element[i].index[2]]-kite.local_cd[kite.element[i].index[1]];
		kite.element[i].normal=cross(v1,v2);	//�O��
		dummy=unitize(kite.element[i].normal);	//���K��

		//�l�p�`�̏d�S
		kite.element[i].cg=((kite.local_cd[kite.element[i].index[0]]+
							kite.local_cd[kite.element[i].index[1]]+
							kite.local_cd[kite.element[i].index[2]]+
							kite.local_cd[kite.element[i].index[3]]))*0.25;
		
		//�l�p�`�̃��[�J�������e���\��
		double Ixx=0.0,Iyy=0.0,Izz=0.0;//�������[�����g��
		double Ixy=0.0,Ixz=0.0,Iyz=0.0;//�������

		Vec3 dif;
		//�l�p�`�v�f�̒�Ӓ���b�����߂�
		dif=kite.local_cd[kite.element[i].index[0]]-kite.local_cd[kite.element[i].index[1]];
		b=norm(dif);
		//�l�p�`�v�f�̍���c�����߂�
		dif=kite.local_cd[kite.element[i].index[0]]-kite.local_cd[kite.element[i].index[3]];
		c=norm(dif);

		//�������[�����g
		//(1/12)*rho*b*c*c*c etc...
		Ixx=0.0833333*KITE_RHO*b*c*c*c;
		Iyy=0.0833333*KITE_RHO*b*b*b*c;
		Izz=0.0833333*KITE_RHO*b*c*(b*b+c*c);

		kite.element[i].local_inertia_mom.data[0]=Ixx;
		kite.element[i].local_inertia_mom.data[1]=Iyy;
		kite.element[i].local_inertia_mom.data[2]=Izz;

		//�������
		//(1/16)*rho*b*b*c*c etc...
		Ixy=-0.0625*KITE_RHO*b*b*c*c;
		Ixz=0.0;
		Iyz=0.0;
		
		kite.element[i].local_inertia_pro.data[0]=Ixy;
		kite.element[i].local_inertia_pro.data[1]=Ixz;
		kite.element[i].local_inertia_pro.data[2]=Iyz;

		//���S�̖̂ʐϥ���ʂ��v�Z
		kite.S+=kite.element[i].S;
		kite.mass+=kite.element[i].mass;
	}

	//���S�̂̏d�S���v�Z
	Vec3 mom=Vec3();
	for(i=0;i<q_num;i++)
	{
		mom+=kite.element[i].mass*kite.element[i].cg;
	}
	double kite_mass_inv=1.0/kite.mass;
	kite.cg=mom*kite_mass_inv;//�d�S

	//�d�S��̍��W�ւƕϊ�
	for(i=0;i<q_num;i++)
	{
		kite.element[i].cg-=kite.cg;
	}
	kite.s-=kite.cg;//���ڒ��S��
	//���[�J�����W��
	for(i=0;i<p_num;i++)
	{
		kite.local_cd[i]-=kite.cg;
	}

	//�����e���\���쐬
	double Ixx,Iyy,Izz,Ixy,Ixz,Iyz;
	Ixx=0.0;Iyy=0.0;Izz=0.0;//�������[�����g��
	Ixy=0.0;Ixz=0.0;Iyz=0.0;//�������
	for(i=0;i<q_num;i++)
	{
		Ixx+=kite.element[i].local_inertia_mom.data[0]+
			kite.element[i].mass*
			(kite.element[i].cg.data[1]*kite.element[i].cg.data[1]+
			kite.element[i].cg.data[2]*kite.element[i].cg.data[2]);
		Iyy+=kite.element[i].local_inertia_mom.data[1]+
			kite.element[i].mass*
			(kite.element[i].cg.data[0]*kite.element[i].cg.data[0]+
			kite.element[i].cg.data[2]*kite.element[i].cg.data[2]);
		Izz+=kite.element[i].local_inertia_mom.data[2]+
			kite.element[i].mass*
			(kite.element[i].cg.data[1]*kite.element[i].cg.data[1]+
			kite.element[i].cg.data[0]*kite.element[i].cg.data[0]);

		Ixy+=kite.element[i].local_inertia_pro.data[0]+
			kite.element[i].mass*
			(kite.element[i].cg.data[0]*kite.element[i].cg.data[1]);
		Ixz+=kite.element[i].local_inertia_pro.data[1]+
			kite.element[i].mass*
			(kite.element[i].cg.data[0]*kite.element[i].cg.data[2]);
		Iyz+=kite.element[i].local_inertia_pro.data[2]+
			kite.element[i].mass*
			(kite.element[i].cg.data[2]*kite.element[i].cg.data[1]);
	}

	kite.inertia.SetValue(Ixx,-Ixy,-Ixz,-Ixy,Iyy,-Iyz,-Ixz,-Iyz,Izz);//�����e���\���Ɋi�[
	kite.inertia_inv=kite.inertia.Inverse();//�t�s��
	//wxLogMessage(_T("%lf %lf %lf"),kite.inertia.element(0,0),kite.inertia.element(0,1),kite.inertia.element(0,2));
	//wxLogMessage(_T("%lf %lf %lf"),kite.inertia.element(1,0),kite.inertia.element(1,1),kite.inertia.element(1,2));
	//wxLogMessage(_T("%lf %lf %lf"),kite.inertia.element(2,0),kite.inertia.element(2,1),kite.inertia.element(2,2));

	kite.glb_s_pos=QVRotate(kite.orientation,kite.s)+kite.pos;

//�����ۂ̃Z�b�g--------------------------------------------------

	//�����ۂ̐ڑ��[�_�ʒu�Z�b�g
	tail[0].set_point=(COL_NUM*(ROW_NUM-1)+1)+1;
	tail[0].pos[0]=kite.global_cd[tail[0].set_point];
	tail[1].set_point=COL_NUM*ROW_NUM-3;
	tail[1].pos[0]=kite.global_cd[tail[1].set_point];

	for(i=0;i<=TAIL_SP;i++)//�v�f�������[�v
	{
		for(j=0;j<TAIL_NUM;j++)//�����ې������[�v
		{
			tail[j].pos[i]=tail[j].pos[0]-Vec3(0.0,0.0,tail[j].l*((double)i)/((double)TAIL_SP));
			tail[j].pos_pre[i]=tail[j].pos[i];
		}
	}
//----------------------------------------------------------------
}

/*!
 * @note ���̃f�U�C�����烂�f���쐬
 */
void 
kite3d::create_model_yak(void)
{
	//�ŏI�I�ɂ͓��͂��ꂽ�f�U�C������
	//�_�Q�ƎO�p�`�|���S�����쐬

	int i=0,j=0,k=0;

//����---------------------------------
	double b=1.0;//�ő剡��
	double c=0.8;//�ő�c��

	vector<Vec3> point;	//�_�Q
	int p_num=0;		//�_�Q��

	vector<Vec3> tex_p;	//�e�N�X�`�����W

	for(int i=0;i<ROW_NUM;i++)//�c����(x����)
	{
		for(int j=0;j<COL_NUM;j++)//������(y����)
		{
			point.push_back(Vec3(((double)i)*c/((double)(ROW_NUM-1)),((double)j)*b/((double)(COL_NUM-1)),0.0 ));
			p_num++;//�o�^�_�����Z
			tex_p.push_back(Vec3( -((double)i)/((double)(ROW_NUM-1)),-((double)j)/((double)(COL_NUM-1)),0.0));
		}
	}

	vector<int> q_index[4];	//�l�p�`���\������4�_�̃C���f�b�N�X
	int q_num=0;			//�l�p�`�̐�

	for(int i=0;i<(ROW_NUM-1);i++)
	{
		for(int j=0;j<(COL_NUM-1);j++)
		{
			if(!((((COL_NUM-1)/4)>j||(3*(COL_NUM-1)/4)<=j)&&(((ROW_NUM-1)/2)<=i)))
			{
				//�l�p�`���i�[���Ă���
				q_index[0].push_back(i*COL_NUM+j);
				q_index[1].push_back(i*COL_NUM+(j+1));
				q_index[2].push_back((i+1)*COL_NUM+(j+1));
				q_index[3].push_back((i+1)*COL_NUM+j);
				q_num++;
			}
		}
	}

	Vec3 itome=Vec3(0.3*c,0.5*b,0.0);//���ڒ��S

	//���͂���v�Z�p���f���쐬
	kite.b=b;
	kite.c=c;
	kite.AR=b/c;

	kite.p_num=p_num;	//���_���i�[
	kite.q_num=q_num;	//�l�p�`�v�f���i�[

	kite.s=itome;		//���ڒ��S��荞��

	for(i=0;i<p_num;i++)
	{
		kite.tex_cd.push_back(tex_p[i]);

		kite.local_cd.push_back(point[i]);	//���[�J�����W�͌�ŏd�S��ɂȂ�

		kite.design_cd.push_back(point[i]-kite.s);//�f�U�C�����W�͎��ڒ��S����ɂ��Ă���
		kite.global_cd.push_back(QVRotate(kite.orientation,kite.design_cd[i]));	//�O���[�o�����W
		kite.global_cd[i]+=kite.pos;
	}

	kite.S=0.0; kite.mass=0.0;	//������
	kite.element.resize(q_num);	//�l�p�`�v�f����element�m��
	for(i=0;i<q_num;i++)
	{
		kite.element[i].b=kite.b;
		kite.element[i].c=kite.c;

		k=i/(COL_NUM-1);//���s�ڂ̎l�p�`�����Ă��邩�`�F�b�N

		if(((k*(COL_NUM-1)+((COL_NUM-1)/4)>i)||(k*(COL_NUM-1)+(3*(COL_NUM-1)/4)<=i))
			&&((((COL_NUM-1)*(ROW_NUM-1))/2)>i))
		{
			kite.element[i].c*=0.5;
		}
		else if((((COL_NUM-1)*(ROW_NUM-1))/2)<=i)
		{
			kite.element[i].b*=0.5;
		}

		//�l�p�`���\������4�_�̃C���f�b�N�X
		kite.element[i].index[0]=(int)q_index[0][i];
		kite.element[i].index[1]=(int)q_index[1][i];
		kite.element[i].index[2]=(int)q_index[2][i];
		kite.element[i].index[3]=(int)q_index[3][i];

		//�w�����̌������l�p�`�̖ʐς��v�Z
		kite.element[i].S=Heron(kite.local_cd[kite.element[i].index[0]],
							kite.local_cd[kite.element[i].index[1]],
							kite.local_cd[kite.element[i].index[2]])
						+ Heron(kite.local_cd[kite.element[i].index[0]],
							kite.local_cd[kite.element[i].index[2]],
							kite.local_cd[kite.element[i].index[3]]);

		//�l�p�`�̎���(�ʐ�*���x)
		kite.element[i].mass=kite.element[i].S*KITE_RHO;

		Vec3 v1,v2;
		double dummy;
		//�l�p�`�̖ʖ@��
		v1=kite.local_cd[kite.element[i].index[1]]-kite.local_cd[kite.element[i].index[0]];
		v2=kite.local_cd[kite.element[i].index[2]]-kite.local_cd[kite.element[i].index[1]];
		kite.element[i].normal=cross(v1,v2);	//�O��
		dummy=unitize(kite.element[i].normal);	//���K��

		//�l�p�`�̏d�S
		kite.element[i].cg=((kite.local_cd[kite.element[i].index[0]]+
							kite.local_cd[kite.element[i].index[1]]+
							kite.local_cd[kite.element[i].index[2]]+
							kite.local_cd[kite.element[i].index[3]]))*0.25;
		
		//�l�p�`�̃��[�J�������e���\��
		double Ixx=0.0,Iyy=0.0,Izz=0.0;//�������[�����g��
		double Ixy=0.0,Ixz=0.0,Iyz=0.0;//�������

		Vec3 dif;
		//�l�p�`�v�f�̒�Ӓ���b�����߂�
		dif=kite.local_cd[kite.element[i].index[0]]-kite.local_cd[kite.element[i].index[1]];
		b=norm(dif);
		//�l�p�`�v�f�̍���c�����߂�
		dif=kite.local_cd[kite.element[i].index[0]]-kite.local_cd[kite.element[i].index[3]];
		c=norm(dif);

		//�������[�����g
		//(1/12)*rho*b*c*c*c etc...
		Ixx=0.0833333*KITE_RHO*b*c*c*c;
		Iyy=0.0833333*KITE_RHO*b*b*b*c;
		Izz=0.0833333*KITE_RHO*b*c*(b*b+c*c);

		kite.element[i].local_inertia_mom.data[0]=Ixx;
		kite.element[i].local_inertia_mom.data[1]=Iyy;
		kite.element[i].local_inertia_mom.data[2]=Izz;

		//�������
		//(1/16)*rho*b*b*c*c etc...
		Ixy=-0.0625*KITE_RHO*b*b*c*c;
		Ixz=0.0;
		Iyz=0.0;
		
		kite.element[i].local_inertia_pro.data[0]=Ixy;
		kite.element[i].local_inertia_pro.data[1]=Ixz;
		kite.element[i].local_inertia_pro.data[2]=Iyz;

		//���S�̖̂ʐϥ���ʂ��v�Z
		kite.S+=kite.element[i].S;
		kite.mass+=kite.element[i].mass;
	}
	//���S�̂̏d�S���v�Z
	Vec3 mom=Vec3();
	for(i=0;i<q_num;i++)
	{
		mom+=kite.element[i].mass*kite.element[i].cg;
	}
	double kite_mass_inv=1.0/kite.mass;
	kite.cg=mom*kite_mass_inv;//�d�S

	//�d�S��̍��W�ւƕϊ�
	for(i=0;i<q_num;i++)
	{
		kite.element[i].cg-=kite.cg;
	}
	kite.s-=kite.cg;//���ڒ��S��
	//���[�J�����W��
	for(i=0;i<p_num;i++)
	{
		kite.local_cd[i]-=kite.cg;
	}

	//�����e���\���쐬
	double Ixx,Iyy,Izz,Ixy,Ixz,Iyz;
	Ixx=0.0;Iyy=0.0;Izz=0.0;//�������[�����g��
	Ixy=0.0;Ixz=0.0;Iyz=0.0;//�������
	for(i=0;i<q_num;i++)
	{
		Ixx+=kite.element[i].local_inertia_mom.data[0]+
			kite.element[i].mass*
			(kite.element[i].cg.data[1]*kite.element[i].cg.data[1]+
			kite.element[i].cg.data[2]*kite.element[i].cg.data[2]);
		Iyy+=kite.element[i].local_inertia_mom.data[1]+
			kite.element[i].mass*
			(kite.element[i].cg.data[0]*kite.element[i].cg.data[0]+
			kite.element[i].cg.data[2]*kite.element[i].cg.data[2]);
		Izz+=kite.element[i].local_inertia_mom.data[2]+
			kite.element[i].mass*
			(kite.element[i].cg.data[1]*kite.element[i].cg.data[1]+
			kite.element[i].cg.data[0]*kite.element[i].cg.data[0]);

		Ixy+=kite.element[i].local_inertia_pro.data[0]+
			kite.element[i].mass*
			(kite.element[i].cg.data[0]*kite.element[i].cg.data[1]);
		Ixz+=kite.element[i].local_inertia_pro.data[1]+
			kite.element[i].mass*
			(kite.element[i].cg.data[0]*kite.element[i].cg.data[2]);
		Iyz+=kite.element[i].local_inertia_pro.data[2]+
			kite.element[i].mass*
			(kite.element[i].cg.data[2]*kite.element[i].cg.data[1]);
	}

	kite.inertia.SetValue(Ixx,-Ixy,-Ixz,-Ixy,Iyy,-Iyz,-Ixz,-Iyz,Izz);//�����e���\���Ɋi�[
	kite.inertia_inv=kite.inertia.Inverse();//�t�s��
	//wxLogMessage(_T("%lf %lf %lf"),kite.inertia.element(0,0),kite.inertia.element(0,1),kite.inertia.element(0,2));
	//wxLogMessage(_T("%lf %lf %lf"),kite.inertia.element(1,0),kite.inertia.element(1,1),kite.inertia.element(1,2));
	//wxLogMessage(_T("%lf %lf %lf"),kite.inertia.element(2,0),kite.inertia.element(2,1),kite.inertia.element(2,2));

	kite.glb_s_pos=QVRotate(kite.orientation,kite.s)+kite.pos;

//�����ۂ̃Z�b�g--------------------------------------------------

	//�����ۂ̐ڑ��[�_�ʒu�Z�b�g
	tail[0].set_point=(COL_NUM*(ROW_NUM-1)+1)+((COL_NUM-1)/4);
	tail[0].pos[0]=kite.global_cd[tail[0].set_point];
	tail[1].set_point=(COL_NUM*ROW_NUM-2)-((COL_NUM-1)/4);
	tail[1].pos[0]=kite.global_cd[tail[1].set_point];

	for(i=0;i<=TAIL_SP;i++)//�v�f�������[�v
	{
		for(j=0;j<TAIL_NUM;j++)//�����ې������[�v
		{
			tail[j].pos[i]=tail[j].pos[0]-Vec3(0.0,0.0,tail[j].l*((double)i)/((double)TAIL_SP));
			tail[j].pos_pre[i]=tail[j].pos[i];
		}
	}
//----------------------------------------------------------------
}

/*!
 * @note ���̃f�U�C�����烂�f���쐬
 */
void 
kite3d::create_model_dia(void)
{
	//�ŏI�I�ɂ͓��͂��ꂽ�f�U�C������
	//�_�Q�ƎO�p�`�|���S�����쐬

	int i=0,j=0,k=0;

//����---------------------------------
	double b=1.0;//�ő剡��
	double c=1.0;//�ő�c��

	vector<Vec3> point;	//�_�Q
	int p_num=0;		//�_�Q��

	vector<Vec3> tex_p;	//�e�N�X�`�����W

	for(int i=0;i<ROW_NUM;i++)//�c����(x����)
	{
		for(int j=0;j<COL_NUM;j++)//������(y����)
		{
			point.push_back(Vec3(((double)i)*c/((double)(ROW_NUM-1)),((double)j)*b/((double)(COL_NUM-1)),0.0 ));
			p_num++;//�o�^�_�����Z
			tex_p.push_back(Vec3( -((double)i)/((double)(ROW_NUM-1)),-((double)j)/((double)(COL_NUM-1)),0.0));
		}
	}

	vector<int> q_index[4];	//�l�p�`���\������4�_�̃C���f�b�N�X
	int q_num=0;			//�l�p�`�̐�

	for(int i=0;i<(ROW_NUM-1);i++)
	{
		for(int j=0;j<(COL_NUM-1);j++)
		{
			if((((COL_NUM/2)-(i+1))<=j)&&(((COL_NUM/2)+(i+1))>j)&&((ROW_NUM/2)>i))
			{
				//�l�p�`���i�[���Ă���
				q_index[0].push_back(i*COL_NUM+j);
				q_index[1].push_back(i*COL_NUM+(j+1));
				q_index[2].push_back((i+1)*COL_NUM+(j+1));
				q_index[3].push_back((i+1)*COL_NUM+j);
				q_num++;
			}
			else if((((COL_NUM/2)-(ROW_NUM-(i+1)))<=j)&&(((COL_NUM/2)+(ROW_NUM-(i+1)))>j)&&((ROW_NUM/2)<=i))
			{
				//�l�p�`���i�[���Ă���
				q_index[0].push_back(i*COL_NUM+j);
				q_index[1].push_back(i*COL_NUM+(j+1));
				q_index[2].push_back((i+1)*COL_NUM+(j+1));
				q_index[3].push_back((i+1)*COL_NUM+j);
				q_num++;
			}
		}
	}

	Vec3 itome=Vec3(0.3*c,0.5*b,0.0);//���ڒ��S

	//���͂���v�Z�p���f���쐬
	kite.b=b;
	kite.c=c;
	kite.AR=b/c;

	kite.p_num=p_num;	//���_���i�[
	kite.q_num=q_num;	//�l�p�`�v�f���i�[

	kite.s=itome;		//���ڒ��S��荞��

	for(i=0;i<p_num;i++)
	{
		kite.tex_cd.push_back(tex_p[i]);

		kite.local_cd.push_back(point[i]);	//���[�J�����W�͌�ŏd�S��ɂȂ�

		kite.design_cd.push_back(point[i]-kite.s);//�f�U�C�����W�͎��ڒ��S����ɂ��Ă���
		kite.global_cd.push_back(QVRotate(kite.orientation,kite.design_cd[i]));	//�O���[�o�����W
		kite.global_cd[i]+=kite.pos;
	}

	kite.S=0.0; kite.mass=0.0;	//������
	kite.element.resize(q_num);	//�l�p�`�v�f����element�m��
	for(i=0;i<q_num;i++)
	{
		kite.element[i].b=kite.b;
		kite.element[i].c=kite.c;

		k=q_index[0][i]/COL_NUM;//���s�ڂ̓_�����Ă��邩�`�F�b�N

		//��b�Ɋւ���
		if((ROW_NUM/2)>k)//�܂�Ԃ��O
		{
			kite.element[i].b*=((double)(k+1))*2.0/((double)(COL_NUM-1));
		}
		else if((ROW_NUM/2)<=k)//�܂�Ԃ���
		{
			kite.element[i].b*=((double)(ROW_NUM-(k+1)))*2.0/((double)(COL_NUM-1));
		}

		//����c�Ɋւ���
		if((COL_NUM/2)>(q_index[0][i]-k*COL_NUM))//�܂�Ԃ��O
		{
			kite.element[i].c*=((double)((q_index[0][i]-k*COL_NUM)+1))*2.0/((double)(ROW_NUM-1));
		}
		else if((COL_NUM/2)<=(q_index[0][i]-k*COL_NUM))//�܂�Ԃ���
		{
			kite.element[i].c*=((double)(ROW_NUM-((q_index[0][i]-k*COL_NUM)+1)))*2.0/((double)(ROW_NUM-1));
		}

		//�l�p�`���\������4�_�̃C���f�b�N�X
		kite.element[i].index[0]=(int)q_index[0][i];
		kite.element[i].index[1]=(int)q_index[1][i];
		kite.element[i].index[2]=(int)q_index[2][i];
		kite.element[i].index[3]=(int)q_index[3][i];

		//�w�����̌������l�p�`�̖ʐς��v�Z
		kite.element[i].S=Heron(kite.local_cd[kite.element[i].index[0]],
							kite.local_cd[kite.element[i].index[1]],
							kite.local_cd[kite.element[i].index[2]])
						+ Heron(kite.local_cd[kite.element[i].index[0]],
							kite.local_cd[kite.element[i].index[2]],
							kite.local_cd[kite.element[i].index[3]]);

		//�l�p�`�̎���(�ʐ�*���x)
		kite.element[i].mass=kite.element[i].S*KITE_RHO;

		Vec3 v1,v2;
		double dummy;
		//�l�p�`�̖ʖ@��
		v1=kite.local_cd[kite.element[i].index[1]]-kite.local_cd[kite.element[i].index[0]];
		v2=kite.local_cd[kite.element[i].index[2]]-kite.local_cd[kite.element[i].index[1]];
		kite.element[i].normal=cross(v1,v2);	//�O��
		dummy=unitize(kite.element[i].normal);	//���K��

		//�l�p�`�̏d�S
		kite.element[i].cg=((kite.local_cd[kite.element[i].index[0]]+
							kite.local_cd[kite.element[i].index[1]]+
							kite.local_cd[kite.element[i].index[2]]+
							kite.local_cd[kite.element[i].index[3]]))*0.25;
		
		//�l�p�`�̃��[�J�������e���\��
		double Ixx=0.0,Iyy=0.0,Izz=0.0;//�������[�����g��
		double Ixy=0.0,Ixz=0.0,Iyz=0.0;//�������

		Vec3 dif;
		//�l�p�`�v�f�̒�Ӓ���b�����߂�
		dif=kite.local_cd[kite.element[i].index[0]]-kite.local_cd[kite.element[i].index[1]];
		b=norm(dif);
		//�l�p�`�v�f�̍���c�����߂�
		dif=kite.local_cd[kite.element[i].index[0]]-kite.local_cd[kite.element[i].index[3]];
		c=norm(dif);

		//�������[�����g
		//(1/12)*rho*b*c*c*c etc...
		Ixx=0.0833333*KITE_RHO*b*c*c*c;
		Iyy=0.0833333*KITE_RHO*b*b*b*c;
		Izz=0.0833333*KITE_RHO*b*c*(b*b+c*c);

		kite.element[i].local_inertia_mom.data[0]=Ixx;
		kite.element[i].local_inertia_mom.data[1]=Iyy;
		kite.element[i].local_inertia_mom.data[2]=Izz;

		//�������
		//(1/16)*rho*b*b*c*c etc...
		Ixy=-0.0625*KITE_RHO*b*b*c*c;
		Ixz=0.0;
		Iyz=0.0;
		
		kite.element[i].local_inertia_pro.data[0]=Ixy;
		kite.element[i].local_inertia_pro.data[1]=Ixz;
		kite.element[i].local_inertia_pro.data[2]=Iyz;

		//���S�̖̂ʐϥ���ʂ��v�Z
		kite.S+=kite.element[i].S;
		kite.mass+=kite.element[i].mass;
	}
	//���S�̂̏d�S���v�Z
	Vec3 mom=Vec3();
	for(i=0;i<q_num;i++)
	{
		mom+=kite.element[i].mass*kite.element[i].cg;
	}
	double kite_mass_inv=1.0/kite.mass;
	kite.cg=mom*kite_mass_inv;//�d�S

	//�d�S��̍��W�ւƕϊ�
	for(i=0;i<q_num;i++)
	{
		kite.element[i].cg-=kite.cg;
	}
	kite.s-=kite.cg;//���ڒ��S��
	//���[�J�����W��
	for(i=0;i<p_num;i++)
	{
		kite.local_cd[i]-=kite.cg;
	}

	//�����e���\���쐬
	double Ixx,Iyy,Izz,Ixy,Ixz,Iyz;
	Ixx=0.0;Iyy=0.0;Izz=0.0;//�������[�����g��
	Ixy=0.0;Ixz=0.0;Iyz=0.0;//�������
	for(i=0;i<q_num;i++)
	{
		Ixx+=kite.element[i].local_inertia_mom.data[0]+
			kite.element[i].mass*
			(kite.element[i].cg.data[1]*kite.element[i].cg.data[1]+
			kite.element[i].cg.data[2]*kite.element[i].cg.data[2]);
		Iyy+=kite.element[i].local_inertia_mom.data[1]+
			kite.element[i].mass*
			(kite.element[i].cg.data[0]*kite.element[i].cg.data[0]+
			kite.element[i].cg.data[2]*kite.element[i].cg.data[2]);
		Izz+=kite.element[i].local_inertia_mom.data[2]+
			kite.element[i].mass*
			(kite.element[i].cg.data[1]*kite.element[i].cg.data[1]+
			kite.element[i].cg.data[0]*kite.element[i].cg.data[0]);

		Ixy+=kite.element[i].local_inertia_pro.data[0]+
			kite.element[i].mass*
			(kite.element[i].cg.data[0]*kite.element[i].cg.data[1]);
		Ixz+=kite.element[i].local_inertia_pro.data[1]+
			kite.element[i].mass*
			(kite.element[i].cg.data[0]*kite.element[i].cg.data[2]);
		Iyz+=kite.element[i].local_inertia_pro.data[2]+
			kite.element[i].mass*
			(kite.element[i].cg.data[2]*kite.element[i].cg.data[1]);
	}

	kite.inertia.SetValue(Ixx,-Ixy,-Ixz,-Ixy,Iyy,-Iyz,-Ixz,-Iyz,Izz);//�����e���\���Ɋi�[
	kite.inertia_inv=kite.inertia.Inverse();//�t�s��
	//wxLogMessage(_T("%lf %lf %lf"),kite.inertia.element(0,0),kite.inertia.element(0,1),kite.inertia.element(0,2));
	//wxLogMessage(_T("%lf %lf %lf"),kite.inertia.element(1,0),kite.inertia.element(1,1),kite.inertia.element(1,2));
	//wxLogMessage(_T("%lf %lf %lf"),kite.inertia.element(2,0),kite.inertia.element(2,1),kite.inertia.element(2,2));

	kite.glb_s_pos=QVRotate(kite.orientation,kite.s)+kite.pos;

//�����ۂ̃Z�b�g--------------------------------------------------

	//�����ۂ̐ڑ��[�_�ʒu�Z�b�g
	tail[0].set_point=COL_NUM*ROW_NUM-1-(COL_NUM/2);
	tail[0].pos[0]=kite.global_cd[tail[0].set_point];
	tail[1].set_point=COL_NUM*ROW_NUM-1-(COL_NUM/2);
	tail[1].pos[0]=kite.global_cd[tail[1].set_point];

	for(i=0;i<=TAIL_SP;i++)//�v�f�������[�v
	{
		for(j=0;j<TAIL_NUM;j++)//�����ې������[�v
		{
			tail[j].pos[i]=tail[j].pos[0]-Vec3(0.0,0.0,tail[j].l*((double)i)/((double)TAIL_SP));
			tail[j].pos_pre[i]=tail[j].pos[i];
		}
	}
//----------------------------------------------------------------
}

/*!
 * Heron�̌���
 * @note		3���_�̍��W����O�p�`�ʐς����߂�
 * @param[in]	A �O�p�`���_(Vec3�^)
 * @param[in]	B �O�p�`���_(Vec3�^)
 * @param[in]	C �O�p�`���_(Vec3�^)
 * @return		�O�p�`�ʐς�Ԃ�(double�^)
 */
double 
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
 * @note ��-CD�e�[�u��(�}���p����R�͌W�������߂�)
 * @param[in] alpha
 * @return CD
 */
double 
kite3d::search_alpha_CD(double alpha,double AR)
{
	double CD[3];
	double terminal;
	int index=(int)alpha;

	if(0>index)//���ł��ꉞ�v�Z�ł���悤��
	{
		CD[0]=CD_068_table[0];
		CD[1]=CD_148_table[0];
	}
	else if(45<=index)//���E�z��
	{
		CD[0]=CD_068_table[45]*(1.0-cos(2.0*alpha*RX_DEGREES_TO_RADIANS));
		CD[1]=CD_148_table[45]*(1.0-cos(2.0*alpha*RX_DEGREES_TO_RADIANS));
	}
	else
	{
		terminal=alpha-(double)index;//��ԗp
		CD[0]=terminal*CD_068_table[index+1]+(1.0-terminal)*CD_068_table[index];//���`���
		CD[1]=terminal*CD_148_table[index+1]+(1.0-terminal)*CD_148_table[index];//���`���
	}

	terminal=(AR-TABLE_S)/(TABLE_L-TABLE_S);
	CD[2]=terminal*CD[0]+(1.0-terminal)*CD[1];//���`���

	return CD[2];
}

/*!
 * @note ��-CL�e�[�u��(�}���p����g�͌W�������߂�)
 * @param[in] alpha
 * @return CL
 */
double 
kite3d::search_alpha_CL(double alpha,double AR)
{
	double CL[3];
	double terminal;
	int index=(int)alpha;

	if(0>index)//���ł��ꉞ�v�Z�ł���悤��
	{
		CL[0]=CL_068_table[0];
		CL[1]=CL_148_table[0];
	}
	else if(90<=index)//���E�z��
	{
		CL[0]=CL_068_table[0];
		CL[1]=CL_148_table[0];
	}
	else if(45<=index&&90>index)//���E�z��
	{
		CL[0]=CL_068_table[45]*sin(2.0*alpha*RX_DEGREES_TO_RADIANS);
		CL[1]=CL_148_table[45]*sin(2.0*alpha*RX_DEGREES_TO_RADIANS);
	}
	else
	{
		terminal=alpha-(double)index;//��ԗp
		CL[0]=terminal*CL_068_table[index+1]+(1.0-terminal)*CL_068_table[index];//���`���
		CL[1]=terminal*CL_148_table[index+1]+(1.0-terminal)*CL_148_table[index];//���`���
	}

	terminal=(AR-TABLE_S)/(TABLE_L-TABLE_S);
	CL[2]=terminal*CL[0]+(1.0-terminal)*CL[1];//���`���

	return CL[2];
}

/*!
 * @note ��-x�e�[�u��(�}���p���畗�S�����߂�)
 * @param[in] alpha
 * @return x
 */
double 
kite3d::search_alpha_x(double alpha)
{
	double x_coefficient;
	double terminal;
	int index=((int)alpha)-20;

	if(0>index)//��-x�Ȑ��̒�������
	{
		x_coefficient=x_table[0];
	}
	else if(70<=index)//���E����
	{
		x_coefficient=x_table[70];
	}
	else
	{
		terminal=alpha-(double)(index+20);//��ԗp
		if(0.01>terminal)
		{
			terminal=0.0;
		}
		else if(1.0<(terminal+0.01))
		{
			terminal=1.0;
		}
		x_coefficient=terminal*x_table[index+1]+(1.0-terminal)*x_table[index];//���`���
	}

	return x_coefficient;
}

/*!
 * @note �V�~�����[�V������i�߂�
 */
void 
kite3d::step_simulation(double dt)
{
	int i=0;
//------------------------
	for(i=0;i<2;i++)
	{
		Lift[0][i]=Vec3();
		Lift[1][i]=Vec3();
		Drag[0][i]=Vec3();
		Drag[1][i]=Vec3();
		T_tail[0][i]=Vec3();
		T_tail[1][i]=Vec3();
	}
//------------------------
	//����
	kite3d::set_wind(dt);//���̃Z�b�g

	//�׏d�ƃ��[�����g�̌v�Z
	kite3d::calc_loads(dt);
	//���i����
	kite.pos+=kite.global_vel*dt+0.5*kite.frc*dt*dt/kite.mass;	//�ʒu�̍X�V
	kite.global_vel+=kite.frc*dt/kite.mass;						//���x�̍X�V
//-----------------------------------------------
#define DAMP_yaw 0.2
#define DAMP 0.0

	//��]�̏���
	//���[�J���n�ɂ�����p���x���v�Z
	Vec3 I_omega=kite.inertia*kite.omega;
	kite.omega+=kite.inertia_inv*(kite.mom-cross(kite.omega,I_omega)-Vec3(DAMP*kite.omega.data[0],DAMP*kite.omega.data[1],DAMP_yaw*kite.omega.data[2]))*dt;
	//�V����4�������v�Z
	kite.orientation+=(kite.orientation*kite.omega)*(0.5*dt);
	//����4�����̐��K��
	kite.orientation.normalize();

//-----------------------------------------------*/
	kite.glb_s_pos=QVRotate(kite.orientation,kite.s)+kite.pos;	//�O���[�o���n�̎��ڈʒu�X�V
	//����
	//����
	kite3d::calc_line_pos(dt);

	kite.glb_s_pos=QVRotate(kite.orientation,kite.s)+kite.pos;	//�O���[�o���n�̎��ڈʒu�X�V

	//�����ۂ̋���
	for(i=0;i<TAIL_NUM;i++)
	{
		kite3d::calc_tail_pos(i,dt);
	}

	//���[�J���n�ɂ����鑬�x���v�Z
	//�܂����̈ړ����x�ƕ������瑊�Α��x�����߂Ă���
	kite.local_vel=kite.global_vel-Wind;								//���Α��x(�����x�x�N�g��-���x�N�g��)
	kite.local_vel=QVRotate(kite.orientation.inverse(),kite.local_vel);	//���[�J���n�փR���o�[�g

	//�I�C���[�p�擾(�ꉞ)
	Vec3 dummy=kite.orientation.GetEulerAngles();
	kite.euler_angles=dummy;

	//�����_�����O�p���W�擾
	kite3d::calc_render_pos();
	
	//�O���[�o��
	G[1]=kite.pos;
	T_string[1]=kite.glb_s_pos;
	for(i=0;i<2;i++)
	{
		//�ʒu
		T_tail[i][1]=kite.global_cd[tail[i].set_point];
		Lift[i][1]=QVRotate(kite.orientation,Lift[i][1])+kite.pos;
		Drag[i][1]=QVRotate(kite.orientation,Drag[i][1])+kite.pos;
		//��
		T_tail[i][0]=QVRotate(kite.orientation,T_tail[i][0]);
		Lift[i][0]=QVRotate(kite.orientation,Lift[i][0]);
		Drag[i][0]=QVRotate(kite.orientation,Drag[i][0]);
	}
}

/*!
 * @note �����_�����O�p���W�擾
 */
void 
kite3d::calc_render_pos(void)
{
	int i=0,j=0;

	//�����
	for(i=0;i<ROW_NUM;i++)
	{
		for(j=0;j<COL_NUM;j++)
		{
			kite.design_cd[(i*COL_NUM)+j].data[2]=p_l[1]-point[j].data[1];
		}
	}

	for(i=0;i<kite.p_num;i++)
	{
		kite.global_cd[i]=QVRotate(kite.orientation,kite.design_cd[i]);	//��]
		kite.global_cd[i]+=kite.glb_s_pos;								//���s�ړ�
	}

}

/*!
 * vector�R���e�i���m�̓��όv�Z
 * @param 
 * @return 
 */
double 
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
int 
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

void 
kite3d::calc_line_pos(double dt)
{
	int i,j,k;

	kite.line_vel[0]=Vec3();

	Vec3 wind_vel=Wind;
	cout<<"wind                              ddasfsd"<<Wind<<endl;
	double wind_norm=0.0;
	wind_norm=unitize(wind_vel);

	double L=kite.l_init/((double)LINE_SP);
	double Inv_dt=1.0/dt;
	Vec3 g_vec=Vec3(0.0,0.0,1.0);
	g_vec*=(L*LINE_RHO)*G_ACCE;

	//���ڈʒu�Ƃ̃����N
	kite.line_vel[LINE_SP]=kite.global_vel;
	kite.line_pos[LINE_SP]=kite.glb_s_pos;

	// 
	//if (gIsAnchorActive)
	//{
		// for Haptic device interface
		kite.line_pos[0]+=kite3d::calc_UI_force()*dt;
	//}
	//else
	//{
		//kite.line_pos[0]-=kite.line_pos_pre[0]*dt;
	//}

	//if(1.0<norm(kite.line_pos[0]))
	//{
	//	kite.line_pos[0]/=norm(kite.line_pos[0]);
	//}

	//������,�R��,�d��
	for(i=0;i<=LINE_SP;i++)
	{
		kite.line_frc[i]=Vec3();
		kite.line_frc[i]+=wind_vel*(wind_norm*wind_norm)*LINE_E;
		kite.line_frc[i]+=g_vec;
	}

	//�΂˂̗�
	for(i=0;i<LINE_SP;i++)
	{
		Vec3 d=kite.line_pos[i]-kite.line_pos[i+1];
		Vec3 vel=kite.line_vel[i]-kite.line_vel[i+1];

		Vec3 f1=-(LINE_K * (norm(d) - L) + LINE_D * ( dot(vel,d) / norm(d) )) * ( d / norm(d) );
		Vec3 f2 = -f1;

		kite.line_frc[i]+=f1;
		kite.line_frc[i+1]+=f2;
	}
	kite.T_spring[0]=kite.line_frc[LINE_SP];

	//���x�E�ʒu�̍X�V
	for(i=1;i<LINE_SP;i++)
	{
		Vec3 Ae=Vec3();

		if(LINE_SP==i)
		{
			Ae=kite.line_frc[i]/(L*LINE_RHO+kite.mass);
		}
		else
		{
			Ae=kite.line_frc[i]/(L*LINE_RHO);
		}

		kite.line_vel[i]+=Ae*dt;
		cout << "line vel ========="<<kite.mass<<endl;
		kite.line_pos[i]+=kite.line_vel[i]*dt;

	}

	//�e���_�̎��ʂ̋t��
	double InvMass[LINE_SP+1];
	for(i=0;i<=LINE_SP;i++)
	{
		if(LINE_SP==i)
		{
			InvMass[i]=1.0/(L*LINE_RHO+kite.mass);
		}
		else
		{
			InvMass[i]=1.0/(L*LINE_RHO);
		}
	}

	//*fast projection
	int flag=0;
	double ee=0.01;//臒l(*100%)
	for(;;)
	{
		flag=0;
		//LINE_SP���΂˂̖{���ɑΉ�
		for(i=0;i<LINE_SP;i++)
		{
			//�ӂ��Ƃ̎�(1)���i�[����
			Vec3 l=kite.line_pos[i+1]-kite.line_pos[i];
			double Inv_E_L=1.0/L;//�΂˂̏������̋t��
			//��(1)
			L_C_strains[i]=norm2(l)*Inv_E_L-L;

			//�����ŁC��(1)���덷�͈͓��Ɏ��܂��Ă��邩�𔻒肷��
			double eps=((1.0+ee)*(1.0+ee)-1.0)*L;
			if(fabs(L_C_strains[i])>eps)//�덷�͈͓��Ɏ��܂��Ă��Ȃ��ꍇ��flag��+����
			{
				flag++;
			}

			//��(2)
			l=2.0*Inv_E_L*(kite.line_pos[i+1]-kite.line_pos[i]);

			int in[6];//index
			//��(2)�̊i�[
			//i�Ԗڂ̎��_�Ɋւ���l
			L_C_grad[i][3*i]=l.data[0];		in[0]=3*i;
			L_C_grad[i][3*i+1]=l.data[1];	in[1]=3*i+1;
			L_C_grad[i][3*i+2]=l.data[2];	in[2]=3*i+2;
			//(i+1)�Ԗڂ̎��_�Ɋւ���l
			L_C_grad[i][3*(i+1)]=-l.data[0];	in[3]=3*(i+1);
			L_C_grad[i][3*(i+1)+1]=-l.data[1];	in[4]=3*(i+1)+1;
			L_C_grad[i][3*(i+1)+2]=-l.data[2];	in[5]=3*(i+1)+2;

			//M�̋t�s��~C_grad�̓]�u�̊i�[
			//i�Ԗڂ̎��_�Ɋւ���l
			L_IM_Cg[3*i][i]=l.data[0]*InvMass[i]*dt*dt;
			L_IM_Cg[3*i+1][i]=l.data[1]*InvMass[i]*dt*dt;
			L_IM_Cg[3*i+2][i]=l.data[2]*InvMass[i]*dt*dt;
			//(i+1)�Ԗڂ̎��_�Ɋւ���l
			L_IM_Cg[3*(i+1)][i]=-l.data[0]*InvMass[i+1]*dt*dt;
			L_IM_Cg[3*(i+1)+1][i]=-l.data[1]*InvMass[i+1]*dt*dt;
			L_IM_Cg[3*(i+1)+2][i]=-l.data[2]*InvMass[i+1]*dt*dt;

			for(k=0;k<LINE_SP;k++)
			{//������
				L_Cg_IM_Cg[LINE_SP*i+k]=0.0;
			}
			//��(5)��A�s�������
			for(j=0;j<LINE_SP;j++)
			{
				for(k=0;k<6;k++)
				{
					L_Cg_IM_Cg[LINE_SP*i+j]+=L_C_grad[i][in[k]]*L_IM_Cg[in[k]][j];
				}
			}
		}

		if(0==flag) break;//�������ׂĂ̕ӂ��덷�͈͓��Ɏ��܂��Ă���̂Ȃ�΁C������ł��؂�

		//�������z�@�ŃɃx�N�g�������߂�
		int bl;
		int m_it=3;
		double cg_eps=1e-5;
		bl=cgSolverMatrix(LINE_SP,L_Cg_IM_Cg,L_C_strains,L_Delta_lambda,m_it,cg_eps);
		//�������󂯎���x�x�N�g��������
		for(i=0;i<3*(LINE_SP+1);i++)
		{
			L_Delta_x[i]=0.0;//������
			
			for(j=0;j<LINE_SP;j++)
			{
				L_Delta_x[i]+=L_IM_Cg[i][j]*L_Delta_lambda[j];
			}
		}
		//�^�̈ʒu�Ɛ^�̑��x
		i=0;
		for(j=0;j<=LINE_SP;j++)
		{
			if(0==j)//�Œ�_�̏���
			{
				i=i+3;

			}
			else if(!(0==j))//�Œ肳��ĂȂ��_�̏���
			{
				kite.line_pos[j].data[0]+=L_Delta_x[i];i++;
				kite.line_pos[j].data[1]+=L_Delta_x[i];i++;
				kite.line_pos[j].data[2]+=L_Delta_x[i];i++;
				kite.line_vel[j]=(kite.line_pos[j]-kite.line_pos_pre[j])*Inv_dt;
			}
		}
	}

	kite.l_now=0.0;//������
	for(i=0;i<LINE_SP;i++)
	{
		kite.l_now+=norm((kite.line_pos[i+1]-kite.line_pos[i]));
	}
	//wxLogMessage(_T("l_now=%lf"),kite.l_now);

	//���ڈʒu�Ƃ̃����N
	kite.glb_s_pos=kite.line_pos[LINE_SP];
	kite.pos=kite.glb_s_pos-QVRotate(kite.orientation,kite.s);
	kite.global_vel=(kite.pos-kite.pos_pre)*Inv_dt;
	kite.pos_pre=kite.pos;
	//*/

//�n�v�e�B�b�N-----------------------------------------------------

	/*/fast projection2
	flag=0;
	ee=0.01;//臒l(*100%)
	for(;;)
	{
		flag=0;
		//LINE_SP���΂˂̖{���ɑΉ�
		for(i=0;i<LINE_SP;i++)
		{
			//�ӂ��Ƃ̎�(1)���i�[����
			Vec3 l=kite.line_pos[i+1]-kite.line_pos[i];
			double Inv_E_L=1.0/L;//�΂˂̏������̋t��
			//��(1)
			L_C_strains[i]=norm2(l)*Inv_E_L-L;

			//�����ŁC��(1)���덷�͈͓��Ɏ��܂��Ă��邩�𔻒肷��
			double eps=((1.0+ee)*(1.0+ee)-1.0)*L;
			if(fabs(L_C_strains[i])>eps)//�덷�͈͓��Ɏ��܂��Ă��Ȃ��ꍇ��flag��+����
			{
				flag++;
			}

			//��(2)
			l=2.0*Inv_E_L*(kite.line_pos[i+1]-kite.line_pos[i]);

			int in[6];//index
			//��(2)�̊i�[
			//i�Ԗڂ̎��_�Ɋւ���l
			L_C_grad[i][3*i]=l.data[0];		in[0]=3*i;
			L_C_grad[i][3*i+1]=l.data[1];	in[1]=3*i+1;
			L_C_grad[i][3*i+2]=l.data[2];	in[2]=3*i+2;
			//(i+1)�Ԗڂ̎��_�Ɋւ���l
			L_C_grad[i][3*(i+1)]=-l.data[0];	in[3]=3*(i+1);
			L_C_grad[i][3*(i+1)+1]=-l.data[1];	in[4]=3*(i+1)+1;
			L_C_grad[i][3*(i+1)+2]=-l.data[2];	in[5]=3*(i+1)+2;

			//M�̋t�s��~C_grad�̓]�u�̊i�[
			//i�Ԗڂ̎��_�Ɋւ���l
			L_IM_Cg[3*i][i]=l.data[0]*InvMass[i]*dt*dt;
			L_IM_Cg[3*i+1][i]=l.data[1]*InvMass[i]*dt*dt;
			L_IM_Cg[3*i+2][i]=l.data[2]*InvMass[i]*dt*dt;
			//(i+1)�Ԗڂ̎��_�Ɋւ���l
			L_IM_Cg[3*(i+1)][i]=-l.data[0]*InvMass[i+1]*dt*dt;
			L_IM_Cg[3*(i+1)+1][i]=-l.data[1]*InvMass[i+1]*dt*dt;
			L_IM_Cg[3*(i+1)+2][i]=-l.data[2]*InvMass[i+1]*dt*dt;

			for(k=0;k<LINE_SP;k++)
			{//������
				L_Cg_IM_Cg[LINE_SP*i+k]=0.0;
			}
			//��(5)��A�s�������
			for(j=0;j<LINE_SP;j++)
			{
				for(k=0;k<6;k++)
				{
					L_Cg_IM_Cg[LINE_SP*i+j]+=L_C_grad[i][in[k]]*L_IM_Cg[in[k]][j];
				}
			}
		}

		if(0==flag) break;//�������ׂĂ̕ӂ��덷�͈͓��Ɏ��܂��Ă���̂Ȃ�΁C������ł��؂�

		//�������z�@�ŃɃx�N�g�������߂�
		int bl;
		int m_it=3;
		double cg_eps=1e-5;
		bl=cgSolverMatrix(LINE_SP,L_Cg_IM_Cg,L_C_strains,L_Delta_lambda,m_it,cg_eps);
		//�������󂯎���x�x�N�g��������
		for(i=0;i<3*(LINE_SP+1);i++)
		{
			L_Delta_x[i]=0.0;//������
			
			for(j=0;j<LINE_SP;j++)
			{
				L_Delta_x[i]+=L_IM_Cg[i][j]*L_Delta_lambda[j];
			}
		}
		//�^�̈ʒu�Ɛ^�̑��x
		i=0;
		for(j=0;j<=LINE_SP;j++)
		{
			if(0==j)//�Œ�_�̏���
			{
				i=i+3;

			}
			else if(!(0==j))//�Œ肳��ĂȂ��_�̏���
			{
				kite.line_pos[j].data[0]+=L_Delta_x[i];i++;
				kite.line_pos[j].data[1]+=L_Delta_x[i];i++;
				kite.line_pos[j].data[2]+=L_Delta_x[i];i++;
				kite.line_vel[j]=(kite.line_pos[j]-kite.line_pos_pre[j])*Inv_dt;
			}
		}
	}

	//���ڈʒu�Ƃ̃����N
	kite.line_pos[LINE_SP]=kite.glb_s_pos;
//-----------------------------------------------------------------*/
	for(i=0;i<=LINE_SP;i++)
	{
		kite.line_pos_pre[i]=kite.line_pos[i];//�R�s�[
		kite.line_vel_pre[i]=kite.line_vel[i];//�R�s�[
	}

	kite.l_check=norm((kite.line_pos[LINE_SP]-kite.line_pos[0]));
}

void 
kite3d::calc_tail_pos(int n,double dt)
{
	int i=0,j=0,k=0;

	//Vec3 wind_vel=Wind-kite.global_vel;
	Vec3 wind_vel=Vec3();
	double wind_norm=0.0;

	double L=tail[n].l/((double)TAIL_SP);
	double Inv_dt=1.0/dt;

	Vec3 g_vec=Vec3(0.0,0.0,1.0);
	g_vec*=(L*TAIL_RHO)*G_ACCE;

	//�ڑ��[�_���x�̎Z�o
	tail[n].vel[0]=kite.global_vel;	//���x

	tail[n].pos[0]=kite.global_cd[tail[n].set_point];	//�ʒu

	//������
	for(i=0;i<=TAIL_SP;i++)
	{
		tail[n].frc[i]=Vec3();
	}

	//�R��,�d�͂�������
	for(i=0;i<=TAIL_SP;i++)
	{
		wind_vel=Wind-tail[n].vel[i];
		wind_norm=unitize(wind_vel);

		tail[n].frc[i]+=wind_vel*(wind_norm*wind_norm)*TAIL_E;
		tail[n].frc[i]+=g_vec;
	}

	//�΂˂̗�
	for(i=0;i<TAIL_SP;i++)
	{
		Vec3 d=tail[n].pos[i]-tail[n].pos[i+1];
		Vec3 vel=tail[n].vel[i]-tail[n].vel[i+1];

		Vec3 f1=-(TAIL_K * (norm(d) - L) + TAIL_D * ( dot(vel,d) / norm(d) )) * ( d / norm(d) );
		Vec3 f2 = -f1;

		tail[n].frc[i]+=f1;
		tail[n].frc[i+1]+=f2;
	}

	//kite.T_spring[1]=tail[n].frc[0];

	//���x�E�ʒu�̍X�V
	for(i=1;i<=TAIL_SP;i++)
	{
		Vec3 Ae=Vec3();

		Ae=tail[n].frc[i]/(L*LINE_RHO);

		tail[n].pos[i]+=tail[n].vel[i]*dt+0.5*Ae*dt*dt;
		tail[n].vel[i]+=Ae*dt;
	}

	//�e���_�̎��ʂ̋t��
	double InvMass[TAIL_SP+1];
	for(i=0;i<=TAIL_SP;i++)
	{
		if(0==i)
		{
			InvMass[i]=1.0/(L*TAIL_RHO+kite.mass);
		}
		else
		{
			InvMass[i]=1.0/(L*TAIL_RHO);
		}
	}

	//*fast projection
	int flag=0;
	double ee=0.01;//臒l(*100%)
	for(;;)
	{
		flag=0;
		//TAIL_SP���΂˂̖{���ɑΉ�
		for(i=0;i<TAIL_SP;i++)
		{
			//�ӂ��Ƃ̎�(1)���i�[����
			Vec3 l=tail[n].pos[i+1]-tail[n].pos[i];
			double Inv_E_L=1.0/L;//�΂˂̏������̋t��
			//��(1)
			T_C_strains[i]=norm2(l)*Inv_E_L-L;

			//�����ŁC��(1)���덷�͈͓��Ɏ��܂��Ă��邩�𔻒肷��
			double eps=((1.0+ee)*(1.0+ee)-1.0)*L;
			if(fabs(T_C_strains[i])>eps)//�덷�͈͓��Ɏ��܂��Ă��Ȃ��ꍇ��flag��+����
			{
				flag++;
			}

			//��(2)
			l=2.0*Inv_E_L*(tail[n].pos[i+1]-tail[n].pos[i]);

			int in[6];//index
			//��(2)�̊i�[
			//i�Ԗڂ̎��_�Ɋւ���l
			T_C_grad[i][3*i]=l.data[0];		in[0]=3*i;
			T_C_grad[i][3*i+1]=l.data[1];	in[1]=3*i+1;
			T_C_grad[i][3*i+2]=l.data[2];	in[2]=3*i+2;
			//(i+1)�Ԗڂ̎��_�Ɋւ���l
			T_C_grad[i][3*(i+1)]=-l.data[0];	in[3]=3*(i+1);
			T_C_grad[i][3*(i+1)+1]=-l.data[1];	in[4]=3*(i+1)+1;
			T_C_grad[i][3*(i+1)+2]=-l.data[2];	in[5]=3*(i+1)+2;

			//M�̋t�s��~C_grad�̓]�u�̊i�[
			//i�Ԗڂ̎��_�Ɋւ���l
			T_IM_Cg[3*i][i]=l.data[0]*InvMass[i]*dt*dt;
			T_IM_Cg[3*i+1][i]=l.data[1]*InvMass[i]*dt*dt;
			T_IM_Cg[3*i+2][i]=l.data[2]*InvMass[i]*dt*dt;
			//(i+1)�Ԗڂ̎��_�Ɋւ���l
			T_IM_Cg[3*(i+1)][i]=-l.data[0]*InvMass[i+1]*dt*dt;
			T_IM_Cg[3*(i+1)+1][i]=-l.data[1]*InvMass[i+1]*dt*dt;
			T_IM_Cg[3*(i+1)+2][i]=-l.data[2]*InvMass[i+1]*dt*dt;

			for(k=0;k<TAIL_SP;k++)
			{//������
				T_Cg_IM_Cg[TAIL_SP*i+k]=0.0;
			}
			//��(5)��A�s�������
			for(j=0;j<TAIL_SP;j++)
			{
				for(k=0;k<6;k++)
				{
					T_Cg_IM_Cg[TAIL_SP*i+j]+=T_C_grad[i][in[k]]*T_IM_Cg[in[k]][j];
				}
			}
		}
		if(0==flag) break;//�������ׂĂ̕ӂ��덷�͈͓��Ɏ��܂��Ă���̂Ȃ�΁C������ł��؂�

		//�������z�@�ŃɃx�N�g�������߂�
		int bl;
		int m_it=3;
		double cg_eps=1e-5;
		bl=cgSolverMatrix(TAIL_SP,T_Cg_IM_Cg,T_C_strains,T_Delta_lambda,m_it,cg_eps);
		//�������󂯎���x�x�N�g��������
		for(i=0;i<3*(TAIL_SP+1);i++)
		{
			T_Delta_x[i]=0.0;//������
			
			for(j=0;j<TAIL_SP;j++)
			{
				T_Delta_x[i]+=T_IM_Cg[i][j]*T_Delta_lambda[j];
			}
		}
		//�^�̈ʒu�Ɛ^�̑��x
		i=0;
		for(j=0;j<=TAIL_SP;j++)
		{

			if(0==j)//�Œ�_�̏���
			{
				i=i+3;

			}
			else if(!(0==j))//�Œ肳��ĂȂ��_�̏���
			{
				tail[n].pos[j].data[0]+=T_Delta_x[i];i++;
				tail[n].pos[j].data[1]+=T_Delta_x[i];i++;
				tail[n].pos[j].data[2]+=T_Delta_x[i];i++;
				tail[n].vel[j]=(tail[n].pos[j]-tail[n].pos_pre[j])*Inv_dt;
			}
		}
	}//*/

	for(i=0;i<=TAIL_SP;i++)
	{
		tail[n].pos_pre[i]=tail[n].pos[i];//�R�s�[
		tail[n].vel_pre[i]=tail[n].vel[i];//�R�s�[
	}
}

/*!
 * @note ���̃Z�b�g
 */
void 
kite3d::set_wind(double dt)
{
#define WIND_STR 6.0//*fabs(sin(dt*StepNo))

	/*/
	//if(StepNo<400)
	if(Z_wind==1)//Z_wind==1
	{
		Wind=Vec3(WIND_STR,0.0,0.0);
	}
	//else if(StepNo>=400)
	else if(X_wind==1)
	{
		Wind=Vec3(0.0,WIND_STR,0.0);
	}
	//*/

//*----------------------------------------------------
	int i,j,k;	//�J�E���^
	int N=GRID;	//�O���b�h��
	double h;	//�O���b�h��

	h = (kite.l_init+1.0)*2.0/(double)N;

	double x,y,z;
	double ef=1.0;

	Vec3 kite_pos=Vec3(-kite.pos.data[1],kite.pos.data[2],kite.pos.data[0]);

	for(i=1;i<=N;i++){//x���W
		if( -kite.l_init+(i-1)*h<kite_pos.data[0] && kite_pos.data[0]<=-kite.l_init+i*h ){
			for(j=1;j<=N;j++){//y���W
				if( -kite.l_init+(j-1)*h<kite_pos.data[1] && kite_pos.data[1]<=-kite.l_init+j*h ){
					for(k=1;k<=N;k++){//z���W
						if( -kite.l_init+(k-1)*h<kite_pos.data[2] && kite_pos.data[2]<=-kite.l_init+k*h ){

							x= g_w[IX(i,j,k)];
							y=-g_u[IX(i,j,k)];
							z= g_v[IX(i,j,k)];
														cout<<"111111111111111111111111111111111111111          "<<IX(i,j,k)<<"   "<<j<<endl;
							Wind=ef*kite.l_init*2.0*Vec3(x,y,z);//�C���x�N�g���Z�b�g

							//wxLogMessage(_T("%lf %lf %lf"),Wind.data[0],Wind.data[1],Wind.data[2]);
						}
					}
				}
			}
		}
	}

//----------------------------------------------------*/
}

void 
kite3d::calc_loads(double dt)
{
	//�׏d�ƃ��[�����g�̏�����
	kite.frc=Vec3();
	kite.mom=Vec3();

	int i=0;	//�J�E���^

	Vec3 Fb,Mb;
	Fb=Vec3();	//���͂��i�[����
	Mb=Vec3();	//���[�����g�̘a���i�[����

	double Fb_nrm=0.0;//���̖͂@����������

	//�F�X�Ȓl���i�[����
	double tmp=0.0;
	Vec3 tmp_vec=Vec3();

	Vec3 local_vel=Vec3();	//���[�J���n�ɂ����鑬�x
	Vec3 xz_vel=Vec3();		//x-z���ʂɂ����鑬�x
	double xz_speed=0.0;	//���x�̑傫��
	Vec3 yz_vel=Vec3();		//y-z���ʂɂ����鑬�x
	double yz_speed=0.0;	//���x�̑傫��

	//Vec3 force_vec=Vec3();	//���͂̉�������
	Vec3 Lift_vec=Vec3();	//�g�͂̉�������
	Vec3 Drag_vec=Vec3();	//�R�͂̉�������
	Vec3 L=Vec3();			//�g��
	Vec3 D=Vec3();			//�R��

	double alpha=0.0;	//�}���p
	double psi=0.0;		//�}���p
	double CD=0.0;		//�R�͌W��
	double CL=0.0;		//�g�͌W��
	double cw=0.0;		//�������S�W��
	Vec3 x_wind=Vec3();	//�������S

	for(i=0;i<kite.q_num;i++)
	{
		//�l�p�`�v�f�̑��x���v�Z
		tmp_vec=cross(kite.omega,kite.element[i].cg);	//�p���x����
		local_vel=kite.local_vel+tmp_vec;				//���[�J���n�ɂ����鑬�x

		//x-z���ʂ�y-z���ʂɑ��x�𓊉e����
		xz_vel=local_vel;	//���x�̃R�s�[
		xz_vel.data[1]=0.0;	//y�����̏���
		yz_vel=local_vel;	//���x�̃R�s�[
		yz_vel.data[0]=0.0;	//x�����̏���

//xz-------------------------------------------

		//�R�͂̉��������̎擾
		xz_speed=unitize(xz_vel);//���x�̑傫���̎擾�Ƒ��x�x�N�g���̐��K��
		Drag_vec=-xz_vel;

		//�g�͂̍�p��������̎擾
		Lift_vec=cross(Drag_vec,-kite.element[i].normal);
		Lift_vec=cross(Lift_vec,Drag_vec);
		tmp=unitize(Lift_vec);//���K��

		//�}���p�𒲂ׂ�
		tmp=dot(Drag_vec,kite.element[i].normal);//cos(alpha)�����߂�
		if(1.0<tmp)			//�{�����肦�Ȃ���
		{					//cos(alpha)��1.0���������ꍇ�̏C��
			tmp=1.0;
		}
		else if(-1.0>tmp)	//�{�����肦�Ȃ���
		{					//cos(alpha)��-1.0����������ꍇ�̏C��
			tmp=-1.0;
		}
		alpha=RX_TO_DEGREES(asin(tmp));	//alpha�����߂�
		if(0.0>alpha)	//�@�������̋t�Ɉړ����Ă���ꍇ
		{				//(����������\�Ɍ������ė���Ă���ꍇ)
			alpha=-alpha;
		}
//*
		//�}���p����`���R�W���ƕ������S���W�����߂�
		cw=kite3d::search_alpha_x(alpha);//�W��
		CD=kite3d::search_alpha_CD(alpha,kite.AR);
		CL=kite3d::search_alpha_CL(alpha,kite.AR);

		//�g�́C�R�͂��v�Z
		L=0.5*CL*RHO*(kite.element[i].S*p_l[0])*(xz_speed*xz_speed)*Lift_vec;
		D=0.5*CD*RHO*(kite.element[i].S*p_l[0])*(xz_speed*xz_speed)*Drag_vec;

		//���S�̂ł̍��͂��v�Z
		Fb+=L+D;

		Lift[0][0]+=L;
		Drag[0][0]+=D;

		//���S�̂ł̃��[�����g���v
		if(0.0<xz_vel.data[0])
		{
			cw=1.0-cw;
		}
		x_wind=Vec3((cw-0.5)*kite.element[i].c+kite.element[i].cg.data[0], kite.element[i].cg.data[1], 0.0);

		tmp_vec=cross(x_wind,L);
		Mb+=tmp_vec;
		tmp_vec=cross(x_wind,D);
		Mb+=tmp_vec;

		Lift[0][1]=Vec3((cw)*kite.element[i].c-kite.cg.data[0],0.0,0.0);
		Drag[0][1]=Vec3((cw)*kite.element[i].c-kite.cg.data[0],0.0,0.0);

		//���̖͂@����������
		Fb_nrm+=dot((L+D),-kite.element[i].normal);

//yz-------------------------------------------*/

//*
		//�R�͂̉��������̎擾
		yz_speed=unitize(yz_vel);//���x�̑傫���̎擾�Ƒ��x�x�N�g���̐��K��
		Drag_vec=-yz_vel;

		//�g�͂̍�p��������̎擾
		Lift_vec=cross(Drag_vec,-kite.element[i].normal);
		Lift_vec=cross(Lift_vec,Drag_vec);
		tmp=unitize(Lift_vec);//���K��

		//�}���p�𒲂ׂ�
		tmp=dot(Drag_vec,kite.element[i].normal);//cos(psi)�����߂�
		if(1.0<tmp)			//�{�����肦�Ȃ���
		{					//cos(psi)��1.0���������ꍇ�̏C��
			tmp=1.0;
		}
		else if(-1.0>tmp)	//�{�����肦�Ȃ���
		{					//cos(psi)��-1.0����������ꍇ�̏C��
			tmp=-1.0;
		}
		psi=RX_TO_DEGREES(asin(tmp));	//psi�����߂�
		if(0.0>psi)
		{
			psi=-psi;
		}

		//�}���p����`���R�W���ƕ��S���W�����߂�
		//�}���p����`���R�W���ƕ������S���W�����߂�
		cw=kite3d::search_alpha_x(psi);//�W��
		CD=kite3d::search_alpha_CD(psi,kite.AR);
		CL=kite3d::search_alpha_CL(psi,kite.AR);

//#define PSI_SP 0.02734
//
//		cw+=PSI_SP;//alpha�p�̊֐���psi�ɗ��p���邽�߂̏C��

		//�g�́C�R�͂��v�Z
		L=0.5*CL*RHO*(kite.element[i].S*p_l[0])*(yz_speed*yz_speed)*Lift_vec;
		D=0.5*CD*RHO*(kite.element[i].S*p_l[0])*(yz_speed*yz_speed)*Drag_vec;

		//���S�̂ł̍��͂��v�Z
		Fb+=L+D;
		Lift[1][0]+=L;
		Drag[1][0]+=D;

		//���S�̂ł̃��[�����g���v
		if(0.0<yz_vel.data[1])
		{
			cw=1.0-cw;
		}
		x_wind=Vec3(kite.element[i].cg.data[0], (cw-0.5)*kite.element[i].b+kite.element[i].cg.data[1], 0.0);

		kite_check=Vec3(0.0,(cw)*kite.b-kite.cg.data[1],0.0);
		tmp_vec=cross(x_wind,L);
		Mb+=tmp_vec;
		tmp_vec=cross(x_wind,D);
		Mb+=tmp_vec;

		Lift[1][1]=Vec3(0.0,(cw)*kite.b-kite.cg.data[1],0.0);
		Drag[1][1]=Vec3(0.0,(cw)*kite.b-kite.cg.data[1],0.0);

		//���̖͂@����������
		Fb_nrm+=dot((L+D),-kite.element[i].normal);

//---------------------------------------------*/
	}
	kite_check=QVRotate(kite.orientation,kite_check);
	
	//����݌v�Z��蓊�e�ʐ�(p_l)�����߂�
	kite3d::calc_deflection(Fb_nrm);

	//�����ۂ���̈�����
	double ce=0.0;
	ce=1.2;
	Vec3 tail_frc=Vec3();
	for(i=0;i<TAIL_NUM;i++)
	{
		tail_frc=QVRotate(kite.orientation.inverse(),tail[i].frc[0]);//�R���o�[�g
		Fb+=ce*tail_frc;
		Mb+=cross(kite.local_cd[tail[i].set_point],ce*tail_frc);

		T_tail[i][0]+=ce*tail_frc;
	}

	//�׏d�����[�J���n����O���[�o���n�ւƃR���o�[�g
	kite.frc=QVRotate(kite.orientation,Fb);

	//���[�����g
	kite.mom+=Mb;

	//�d��(�O���[�o���n�ŉ�����)
	Vec3 g_vec=Vec3(0.0,0.0,1.0);
	g_vec*=kite.mass*G_ACCE;
	kite.frc+=g_vec;

	G[0]=g_vec;//�O���[�o��

	//1step�O�̒��͂����͂ɉ��Z
	kite.frc+=kite.T_spring[0];
	T_string[0]=kite.T_spring[0];

	//���͂ɂ�郂�[�����g
	kite.T_spring[0]=QVRotate(kite.orientation.inverse(),kite.T_spring[0]);//�O���[�o��->���[�J��
	Mb=cross(kite.s,kite.T_spring[0]);//���͂ɂ�郂�[�����g
	kite.mom+=Mb;
}

double 
kite3d::calc_ex_Spring(double T,double dt)
{
	double K=0.2;
	double D=0.2;
	double F[3]={0};

	double d=ex_Sp_pos[0]-ex_Sp_pos[1];
	double vel=ex_Sp_vel[0]-ex_Sp_vel[1];

	F[0]= -(K * (d - ex_Sp_l)+ D * (vel*d) / d );
	F[1]= -F[0];

	F[2]=F[1];

	F[0]-=0.0*T;
	F[1]+=1.0*T;

	for(int i=0;i<2;i++)
	{
		double Ae=0.0;
		Ae=F[i]*1.0;

		if(1!=i)
		{
			ex_Sp_vel[i]+=Ae*dt;
			ex_Sp_pos[i]+=ex_Sp_vel[i]*dt;
		}
	}
	ex_Sp_pos[0]=0.0;

	return F[2];
}

Vec3 
kite3d::calc_UI_force(void)
{
	Vec3 UI_vec=Vec3();
	double ce=0.03;

	UI_vec.data[0]=spring_ce[0]*ce;
	UI_vec.data[1]=-spring_ce[2]*ce;
	UI_vec.data[2]=spring_ce[1]*ce;

	return UI_vec;
}


int 
kite3d::read_file(const string &file_name, vector<double> &datas)
{
	// �g���q�`�F�b�N
	string file_ext = file_name.substr(file_name.size()-3, file_name.size()-1);

	int n=-1;
	int m=0;

	// �t�@�C������̓ǂݍ���
	if(file_ext == "dat"){	// �_�f�[�^�̓ǂݍ���
		double x;
	
		//open
		FILE *fp;
		if((fp = fopen(file_name.c_str(), "r")) == NULL) return -1;

		//�f�[�^��
		fscanf(fp, "%d", &n);
		if(0>n||4<n) return -1;

		if(0==n)
		{
			m=91;
		}
		else
		{
			m=46;
		}
	
		int count = 0;
		for(int i = 0; i < m; ++i){
			fscanf(fp, "%lf", &x);
			datas.push_back(x);
			count++;
		}
		fclose(fp);

		return n;
	}
	else{
		return -1;
	}

}

void 
kite3d::read_file(const string &file_name)
{
	vector<double> datas;
	int i;

	int number=kite3d::read_file(file_name, datas);

	if(0==number)
	{
		for(i=0;i<=70;i++)
		{
			x_table[i]=datas[i];
			x_table[i]+=0.25+0.02734;
		}
	}
	else if(1==number)
	{
		for(i=0;i<=45;i++)
		{
			CD_068_table[i]=datas[i];
		}
	}
	else if(2==number)
	{
		for(i=0;i<=45;i++)
		{
			CD_148_table[i]=datas[i];
		}
	}
	else if(3==number)
	{
		for(i=0;i<=45;i++)
		{
			CL_068_table[i]=datas[i];
		}
	}
	else if(4==number)
	{
		for(i=0;i<=45;i++)
		{
			CL_148_table[i]=datas[i];
		}
	}
}






void 
kite3d::draw_kite(void)
{
	int i=0,j=0,k=0;

	//���̕`��
	/*/
	//�H�`�p
	if(D_tex==1)
	{
		j=0;
		k=0;
		for(i=0;i<kite.q_num;i++)
		{
			if(((2*j)==i)&&((kite.q_num/2)>i))
			{
				glBegin(GL_TRIANGLES);
					glTexCoord2d(kite.tex_cd[kite.element[i].index[1]].data[1],kite.tex_cd[kite.element[i].index[1]].data[0]);
					glVertex3d(kite.global_cd[kite.element[i].index[1]].data[0],kite.global_cd[kite.element[i].index[1]].data[2],-kite.global_cd[kite.element[i].index[1]].data[1]);

					glTexCoord2d(kite.tex_cd[kite.element[i].index[2]].data[1],kite.tex_cd[kite.element[i].index[2]].data[0]);
					glVertex3d(kite.global_cd[kite.element[i].index[2]].data[0],kite.global_cd[kite.element[i].index[2]].data[2],-kite.global_cd[kite.element[i].index[2]].data[1]);

					glTexCoord2d(kite.tex_cd[kite.element[i].index[3]].data[1],kite.tex_cd[kite.element[i].index[3]].data[0]);
					glVertex3d(kite.global_cd[kite.element[i].index[3]].data[0],kite.global_cd[kite.element[i].index[3]].data[2],-kite.global_cd[kite.element[i].index[3]].data[1]);
				glEnd();

				k++;
				j+=k;
			}
			else if(((2*j-1)==i)&&((kite.q_num/2)>i))
			{
				glBegin(GL_TRIANGLES);
					glTexCoord2d(kite.tex_cd[kite.element[i].index[0]].data[1],kite.tex_cd[kite.element[i].index[0]].data[0]);
					glVertex3d(kite.global_cd[kite.element[i].index[0]].data[0],kite.global_cd[kite.element[i].index[0]].data[2],-kite.global_cd[kite.element[i].index[0]].data[1]);

					glTexCoord2d(kite.tex_cd[kite.element[i].index[2]].data[1],kite.tex_cd[kite.element[i].index[2]].data[0]);
					glVertex3d(kite.global_cd[kite.element[i].index[2]].data[0],kite.global_cd[kite.element[i].index[2]].data[2],-kite.global_cd[kite.element[i].index[2]].data[1]);

					glTexCoord2d(kite.tex_cd[kite.element[i].index[3]].data[1],kite.tex_cd[kite.element[i].index[3]].data[0]);
					glVertex3d(kite.global_cd[kite.element[i].index[3]].data[0],kite.global_cd[kite.element[i].index[3]].data[2],-kite.global_cd[kite.element[i].index[3]].data[1]);
				glEnd();
			}
			else if((kite.q_num/2)==i)//�܂�Ԃ�
			{
				glBegin(GL_TRIANGLES);
					glTexCoord2d(kite.tex_cd[kite.element[i].index[0]].data[1],kite.tex_cd[kite.element[i].index[0]].data[0]);
					glVertex3d(kite.global_cd[kite.element[i].index[0]].data[0],kite.global_cd[kite.element[i].index[0]].data[2],-kite.global_cd[kite.element[i].index[0]].data[1]);

					glTexCoord2d(kite.tex_cd[kite.element[i].index[1]].data[1],kite.tex_cd[kite.element[i].index[1]].data[0]);
					glVertex3d(kite.global_cd[kite.element[i].index[1]].data[0],kite.global_cd[kite.element[i].index[1]].data[2],-kite.global_cd[kite.element[i].index[1]].data[1]);

					glTexCoord2d(kite.tex_cd[kite.element[i].index[2]].data[1],kite.tex_cd[kite.element[i].index[2]].data[0]);
					glVertex3d(kite.global_cd[kite.element[i].index[2]].data[0],kite.global_cd[kite.element[i].index[2]].data[2],-kite.global_cd[kite.element[i].index[2]].data[1]);
				glEnd();

				j+=k;
			}
			else if(((2*j)==i)&&((kite.q_num/2)<i))
			{
				glBegin(GL_TRIANGLES);
					glTexCoord2d(kite.tex_cd[kite.element[i].index[0]].data[1],kite.tex_cd[kite.element[i].index[0]].data[0]);
					glVertex3d(kite.global_cd[kite.element[i].index[0]].data[0],kite.global_cd[kite.element[i].index[0]].data[2],-kite.global_cd[kite.element[i].index[0]].data[1]);

					glTexCoord2d(kite.tex_cd[kite.element[i].index[1]].data[1],kite.tex_cd[kite.element[i].index[1]].data[0]);
					glVertex3d(kite.global_cd[kite.element[i].index[1]].data[0],kite.global_cd[kite.element[i].index[1]].data[2],-kite.global_cd[kite.element[i].index[1]].data[1]);

					glTexCoord2d(kite.tex_cd[kite.element[i].index[2]].data[1],kite.tex_cd[kite.element[i].index[2]].data[0]);
					glVertex3d(kite.global_cd[kite.element[i].index[2]].data[0],kite.global_cd[kite.element[i].index[2]].data[2],-kite.global_cd[kite.element[i].index[2]].data[1]);
				glEnd();

				k--;
				j+=k;
			}
			else if(((2*j-1)==i)&&((kite.q_num/2)<i))
			{
				glBegin(GL_TRIANGLES);
					glTexCoord2d(kite.tex_cd[kite.element[i].index[0]].data[1],kite.tex_cd[kite.element[i].index[0]].data[0]);
					glVertex3d(kite.global_cd[kite.element[i].index[0]].data[0],kite.global_cd[kite.element[i].index[0]].data[2],-kite.global_cd[kite.element[i].index[0]].data[1]);

					glTexCoord2d(kite.tex_cd[kite.element[i].index[1]].data[1],kite.tex_cd[kite.element[i].index[1]].data[0]);
					glVertex3d(kite.global_cd[kite.element[i].index[1]].data[0],kite.global_cd[kite.element[i].index[1]].data[2],-kite.global_cd[kite.element[i].index[1]].data[1]);

					glTexCoord2d(kite.tex_cd[kite.element[i].index[3]].data[1],kite.tex_cd[kite.element[i].index[3]].data[0]);
					glVertex3d(kite.global_cd[kite.element[i].index[3]].data[0],kite.global_cd[kite.element[i].index[3]].data[2],-kite.global_cd[kite.element[i].index[3]].data[1]);
				glEnd();
			}
			else
			{
				glBegin(GL_QUADS);
					glTexCoord2d(kite.tex_cd[kite.element[i].index[0]].data[1],kite.tex_cd[kite.element[i].index[0]].data[0]);
					glVertex3d(kite.global_cd[kite.element[i].index[0]].data[0],kite.global_cd[kite.element[i].index[0]].data[2],-kite.global_cd[kite.element[i].index[0]].data[1]);

					glTexCoord2d(kite.tex_cd[kite.element[i].index[1]].data[1],kite.tex_cd[kite.element[i].index[1]].data[0]);
					glVertex3d(kite.global_cd[kite.element[i].index[1]].data[0],kite.global_cd[kite.element[i].index[1]].data[2],-kite.global_cd[kite.element[i].index[1]].data[1]);

					glTexCoord2d(kite.tex_cd[kite.element[i].index[2]].data[1],kite.tex_cd[kite.element[i].index[2]].data[0]);
					glVertex3d(kite.global_cd[kite.element[i].index[2]].data[0],kite.global_cd[kite.element[i].index[2]].data[2],-kite.global_cd[kite.element[i].index[2]].data[1]);

					glTexCoord2d(kite.tex_cd[kite.element[i].index[3]].data[1],kite.tex_cd[kite.element[i].index[3]].data[0]);
					glVertex3d(kite.global_cd[kite.element[i].index[3]].data[0],kite.global_cd[kite.element[i].index[3]].data[2],-kite.global_cd[kite.element[i].index[3]].data[1]);

				glEnd();
			}

		}
	}

	else//*/
	{
		for(i=0;i<kite.q_num;i++)
		{
			glBegin(GL_QUADS);
				//glNormal3d(-kite.element[i].normal[1],kite.element[i].normal[2],kite.element[i].normal[0]);
				glTexCoord2d(kite.tex_cd[kite.element[i].index[0]][1], kite.tex_cd[kite.element[i].index[0]][0]);
				glVertex3d(kite.global_cd[kite.element[i].index[0]][0], kite.global_cd[kite.element[i].index[0]][2], -kite.global_cd[kite.element[i].index[0]][1]);

				//glNormal3d(-kite.element[i].normal[1],kite.element[i].normal[2],kite.element[i].normal[0]);
				glTexCoord2d(kite.tex_cd[kite.element[i].index[1]][1], kite.tex_cd[kite.element[i].index[1]][0]);
				glVertex3d(kite.global_cd[kite.element[i].index[1]][0], kite.global_cd[kite.element[i].index[1]][2], -kite.global_cd[kite.element[i].index[1]][1]);

				//glNormal3d(-kite.element[i].normal[1],kite.element[i].normal[2],kite.element[i].normal[0]);
				glTexCoord2d(kite.tex_cd[kite.element[i].index[2]][1], kite.tex_cd[kite.element[i].index[2]][0]);
				glVertex3d(kite.global_cd[kite.element[i].index[2]][0], kite.global_cd[kite.element[i].index[2]][2], -kite.global_cd[kite.element[i].index[2]][1]);

				//glNormal3d(-kite.element[i].normal[1],kite.element[i].normal[2],kite.element[i].normal[0]);
				glTexCoord2d(kite.tex_cd[kite.element[i].index[3]][1], kite.tex_cd[kite.element[i].index[3]][0]);
				glVertex3d(kite.global_cd[kite.element[i].index[3]][0], kite.global_cd[kite.element[i].index[3]][2], -kite.global_cd[kite.element[i].index[3]][1]);

			glEnd();
		}
	}
}

void 
kite3d::draw_tail(void)
{
	glPushMatrix();
		glDisable(GL_LIGHTING);
		glColor3f ( 0.5f, 0.5f, 0.5f );//gray
		//glColor3f ( 1.0f, 1.0f, 1.0f );//white
		glLineWidth ( 5.0f );

		glBegin ( GL_LINE_STRIP );
		for(int i=0;i<=TAIL_SP;i++)
		{
			glVertex3d ( tail[0].pos[i].data[0],tail[0].pos[i].data[2],-tail[0].pos[i].data[1]);
		}
		glEnd ();

		glBegin ( GL_LINE_STRIP );
		for(int i=0;i<=TAIL_SP;i++)
		{
			glVertex3d ( tail[1].pos[i].data[0],tail[1].pos[i].data[2],-tail[1].pos[i].data[1]);
		}
		glEnd ();

		glEnable(GL_LIGHTING);
	glPopMatrix();
}

void 
kite3d::draw_line(void)
{
	glPushMatrix();
		glDisable(GL_LIGHTING);
		glColor3f ( 1.0f, 0.0f, 0.0f );//red
		glPushMatrix();
		//�͂̍�p�_
			glTranslated(kite.line_pos[0].data[0],kite.line_pos[0].data[2],-kite.line_pos[0].data[1]);
			glutSolidSphere(0.1,15,15);
		glPopMatrix();

		glColor3f ( 0.0f, 0.0f, 0.0f );//black
		glLineWidth ( 1.0f );
		
		//*��{��
		glBegin ( GL_LINE_STRIP );
		for(int i=0;i<=LINE_SP;i++)
		{
			glVertex3d ( kite.line_pos[i].data[0],kite.line_pos[i].data[2],-kite.line_pos[i].data[1] );
		}
		glEnd ();
		//*/
		/*/����
		glBegin ( GL_LINE_STRIP );
		for(int i=0;i<LINE_SP;i++)
		{
			glVertex3d ( kite.line_pos[i].data[0],kite.line_pos[i].data[2],-kite.line_pos[i].data[1] );
		}
		glEnd ();
		for(int i=0;i<4;i++)
		{
			glBegin ( GL_LINES );
				glVertex3d ( kite.line_pos[LINE_SP-1].data[0],kite.line_pos[LINE_SP-1].data[2],-kite.line_pos[LINE_SP-1].data[1] );
				glVertex3d ( kite.global_cd[v_point[i]].data[0],kite.global_cd[v_point[i]].data[2],-kite.global_cd[v_point[i]].data[1] );
			glEnd ();
		}
		//*/
		
		glEnable(GL_LIGHTING);
	glPopMatrix();
}

//�e�͂̍�p�_�\��
void 
kite3d::draw_options_01(void)
{
	glDisable(GL_LIGHTING);

	glColor3f ( 1.0f, 0.0f, 0.0f );//red
	glPushMatrix();
	//�d�S
		glTranslated(G[1].data[0],G[1].data[2],-G[1].data[1]);
		glutSolidSphere(0.03,15,15);
	glPopMatrix();

	glColor3f ( 1.0f, 0.0f, 1.0f );//purple
	glPushMatrix();
	//��p�_1
		glTranslated(Lift[0][1].data[0],Lift[0][1].data[2],-Lift[0][1].data[1]);
		glutSolidSphere(0.03,15,15);
	glPopMatrix();
	glColor3f ( 0.0f, 1.0f, 0.0f );//green
	glPushMatrix();
	//��p�_2
		glTranslated(Lift[1][1].data[0],Lift[1][1].data[2],-Lift[1][1].data[1]);
		glutSolidSphere(0.03,15,15);
	glPopMatrix();

	glColor3f ( 1.0f, 1.0f, 0.0f );//yellow
	glPushMatrix();
	//������1
		glTranslated(T_tail[0][1].data[0],T_tail[0][1].data[2],-T_tail[0][1].data[1]);
		glutSolidSphere(0.03,15,15);
	glPopMatrix();
	glPushMatrix();
	//������2
		glTranslated(T_tail[1][1].data[0],T_tail[1][1].data[2],-T_tail[1][1].data[1]);
		glutSolidSphere(0.03,15,15);
	glPopMatrix();

	glColor3f ( 0.0f, 0.0f, 1.0f );//blue
	glPushMatrix();
	//����
		glTranslated(T_string[1].data[0],T_string[1].data[2],-T_string[1].data[1]);
		glutSolidSphere(0.03,15,15);
	glPopMatrix();

	glEnable(GL_LIGHTING);
}

//�e�͂̑傫���\��
void 
kite3d::draw_options_02(void)
{
	double scale=0.1;
	glDisable(GL_LIGHTING);

	glLineWidth ( 2.0f );
	
	//�d�S
	glColor3f ( 1.0f, 0.0f, 0.0f );//red
	glBegin ( GL_LINES );
		glVertex3d ( G[1].data[0],G[1].data[2],-G[1].data[1] );
		glVertex3d ( G[1].data[0]+scale*G[0].data[0],G[1].data[2]+scale*G[0].data[2],-(G[1].data[1]+scale*G[0].data[1]) );
	glEnd ();

	//�g��
	glColor3f ( 1.0f, 0.0f, 1.0f );//purple
	glBegin ( GL_LINES );
		glVertex3d ( Lift[0][1].data[0],Lift[0][1].data[2],-Lift[0][1].data[1]);
		glVertex3d ( Lift[0][1].data[0]+scale*Lift[0][0].data[0],Lift[0][1].data[2]+scale*Lift[0][0].data[2],-(Lift[0][1].data[1]+scale*Lift[0][0].data[1]));
	glEnd ();
	glBegin ( GL_LINES );
		glVertex3d ( Lift[1][1].data[0],Lift[1][1].data[2],-Lift[1][1].data[1]);
		glVertex3d ( Lift[1][1].data[0]+scale*Lift[1][0].data[0],Lift[1][1].data[2]+scale*Lift[1][0].data[2],-(Lift[1][1].data[1]+scale*Lift[1][0].data[1]));
	glEnd ();
	//�R��
	glColor3f ( 0.0f, 1.0f, 0.0f );//green
	glBegin ( GL_LINES );
		glVertex3d ( Drag[0][1].data[0],Drag[0][1].data[2],-Drag[0][1].data[1]);
		glVertex3d ( Drag[0][1].data[0]+scale*Drag[0][0].data[0],Drag[0][1].data[2]+scale*Drag[0][0].data[2],-(Drag[0][1].data[1]+scale*Drag[0][0].data[1]));
	glEnd ();
	glBegin ( GL_LINES );
		glVertex3d ( Drag[1][1].data[0],Drag[1][1].data[2],-Drag[1][1].data[1]);
		glVertex3d ( Drag[1][1].data[0]+scale*Drag[1][0].data[0],Drag[1][1].data[2]+scale*Drag[1][0].data[2],-(Drag[1][1].data[1]+scale*Drag[1][0].data[1]));
	glEnd ();

	//������1
	glColor3f ( 1.0f, 1.0f, 0.0f );//yellow
	glBegin ( GL_LINES );
		glVertex3d ( T_tail[0][1].data[0],T_tail[0][1].data[2],-T_tail[0][1].data[1]);
		glVertex3d ( T_tail[0][1].data[0]+scale*T_tail[0][0].data[0],T_tail[0][1].data[2]+scale*T_tail[0][0].data[2],-(T_tail[0][1].data[1]+scale*T_tail[0][0].data[1]));
	glEnd ();
	//������2
	glBegin ( GL_LINES );
		glVertex3d ( T_tail[1][1].data[0],T_tail[1][1].data[2],-T_tail[1][1].data[1]);
		glVertex3d ( T_tail[1][1].data[0]+scale*T_tail[1][0].data[0],T_tail[1][1].data[2]+scale*T_tail[1][0].data[2],-(T_tail[1][1].data[1]+scale*T_tail[1][0].data[1]));
	glEnd ();

	//����
	glColor3f ( 1.0f, 0.5f, 0.0f );//orange
	glBegin ( GL_LINES );
		glVertex3d ( T_string[1].data[0],T_string[1].data[2],-T_string[1].data[1]);
		glVertex3d ( T_string[1].data[0]+scale*T_string[0].data[0],T_string[1].data[2]+scale*T_string[0].data[2],-(T_string[1].data[1]+scale*T_string[0].data[1]));
	glEnd ();

	glEnable(GL_LIGHTING);
}

//���͕\��
void 
kite3d::draw_options_03(void)
{
	double scale=0.1;
	glDisable(GL_LIGHTING);

	glColor3f ( 1.0f, 0.0f, 0.0f );//red
	glBegin ( GL_LINES );
		glVertex3d ( G[1].data[0],G[1].data[2],-G[1].data[1] );
		glVertex3d ( G[1].data[0]+scale*kite.frc.data[0],G[1].data[2]+scale*kite.frc.data[2],-(G[1].data[1]+scale*kite.frc.data[1]) );
	glEnd ();

	glEnable(GL_LIGHTING);
}