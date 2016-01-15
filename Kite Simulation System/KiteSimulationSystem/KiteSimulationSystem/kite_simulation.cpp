/*
	a calss to create the kite
	date 2015
*/		

//OpenGL
#include <GL/glew.h>
#include <GL/glut.h>

#include "kite_simulation.h"

LineSimulation line;

double CD_068_table[46];	//!< alpha_CD_068.dat�i�[�p�z��
double CD_148_table[46];	//!< alpha_CD_148.dat�i�[�p�z��
double CL_068_table[46];	//!< alpha_CL_068.dat�i�[�p�z��
double CL_148_table[46];	//!< alpha_CL_148.dat�i�[�p�z��
double x_table[71];		//alpha_x.dat�i�[�p�z��

int nsteps = 0;

//KiteShape kite;
//force, pos
Vec3 Lift[2][2];
Vec3 Drag[2][2];
Vec3 T_string[2];
Vec3 G[2];				//globle position use to draw the force point of the kite

float tension_check;
double ex_Sp_l;
double ex_Sp_pos[2];
double ex_Sp_vel[2];


Vec3 kite_check;
int v_point[4];

Vec3 spring_ce;			//a para to calcualte the user control speed, which can influence the first point of the spring

//deflection
Vec2 point[(BAR_SP+1)];
double sp_l;		//�_�Ԋu
double Wind_vel;	//�C�����x
double p_l[2];		//���e�ʐ�(����)

////���l�v�Z�p
//double Heron(Vec3 A, Vec3 B, Vec3 C);	//�w�����̌���
//double DotProductV(double a[], double b[], int num);
//int cgSolverMatrix(int num, double A[], double b[], double x[], int &max_iter, double &tol);

//init kite func
void KiteSimulation::initKite(void)		//init the para of kite
{
	set_length(8.0);
	//setLineStartPoint(line.getStartPoint());
	//setLineEndPoint(line.getEndPoint());

	kite_check = Vec3();
	tension_check = 0.0;

	ex_Sp_l=1.0;
	ex_Sp_pos[0]=0.0;
	ex_Sp_pos[1]=ex_Sp_l;
	ex_Sp_vel[0]=0.0;
	ex_Sp_vel[1]=0.0;

	//init the line
	line.InitLine();

	//init the kite
	k_pos = line.getEndPoint();			//kite's init pos equals to the end point position of string
	//k_pos = Vec3(-1,0,2);
	k_pos_pre = k_pos;					//pos 1 step ago
	k_global_vel = Vec3();			//global vel
	k_local_vel = Vec3();				//local vel

	k_frc = Vec3();					//�׏d
	k_frc_pre = k_frc;					//previous �׏d
	//k_T_spring[0]	= Vec3();			//�΂˂ɂ�������
	k_T_spring[0]	= line.calTension();	

	k_T_spring[1]	= Vec3();			//�΂˂ɂ�������
	Wind = Vec3();

	//init the rotation para of the kite
	k_omega = Vec3();					//angular velocity 

	//init the direction
	//Initial position (angle)
	double roll  =0.0;
	double pitch = 60.0;
	double yow = 0.0;
	double rdrd = RX_DEGREES_TO_RADIANS* RX_DEGREES_TO_RADIANS;//degree����rad�ɕϊ����邽��
	Vec3 dummy=Vec3(roll*rdrd,pitch*rdrd,yow*rdrd);//(roll,pitch,yow)

	k_orientation.SetEulerAngles(dummy);				//�����p���̊i�[

	for(int i= 0; i< 2; i++)
	{
		Lift[0][i] = Vec3();
		Lift[1][i] = Vec3();

		Drag[0][i] = Vec3();
		Drag[1][i] = Vec3();

		T_string[i] = Vec3();
		G[i] = Vec3();
	}
}

//init the deflection
void KiteSimulation::initialize_deflection(void)	
{
	int i=0;

	for(i= 0;i<=BAR_SP;i++)
	{
		point[i]=Vec2();		//0�ŏ�����
		point[i].data[0] = ((double)i)*k_b/((double)BAR_SP);
	}

	sp_l = k_b/((double)BAR_SP);		//�_�Ԋu
	Wind_vel=0.0;//init the wind

	// Projection area 
	p_l[0] = point[BAR_SP].data[0]-point[0].data[0];
	//max deflection
	p_l[1] = 0.0;
}



void KiteSimulation::calc_deflection(double P)		//calculate the deflection 
{
	p_l[1]=0.0;//init the max deflection

	int i=0,j=0;

	//calc the deflection
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
		var=3.0*k_b*k_b-4.0*point[j].data[0]*point[j].data[0];
		var*=point[j].data[0];

		point[i].data[1]=var*coef[0]*coef[1];

		//�ő傽��݂̃`�F�b�N
		if(point[i].data[1]>p_l[1])
		{
			p_l[1]=point[i].data[1];
		}
	}

	p_l[1]=fabs(p_l[1]);

	keep_long_deflection();
}
void KiteSimulation::keep_long_deflection(void)		//������ۂ�
{
	int i=0;

	double l_2=sp_l*sp_l;//�_�Ԋu��2��
	double y=0.0,y_2=0.0;
	double x=0.0,x_2=0.0;

	for(i=0;i<BAR_SP;i++)
	{
		//calc the y direction of the distance
		y=point[i].data[1]-point[i+1].data[1];
		y=fabs(y);
		
		y_2=y*y;

		//calc the x direction of the distance
		x_2=l_2-y_2;	//(�Ε�^2-y��������^2)
		x=sqrt(x_2);
		x=fabs(x);		//�O�̂��ߐ�Βl�v�Z�ɂ�萳�ɂ���
		
		point[i+1].data[0]=point[i].data[0]+x;
	}

	p_l[0]=point[BAR_SP].data[0]-point[0].data[0];//projected area
}

void KiteSimulation::create_model_rec(void)		//create the model
{
	//�ŏI�I�ɂ͓��͂��ꂽ�f�U�C������
	//�_�Q�ƎO�p�`�|���S�����쐬
	int i=0,j=0;//counter

	//input---------------------------------
	double b_max=0.8;//max kite width
	double c_max=1.2;//max kite height

	vector<Vec3> pointGroups;	//Point groups
	int pointgroup_num=0;		//number of the point groups

	vector<Vec3> tex_p;	//texture pos

	for(i=0;i<ROW_NUM;i++)//x direction
	{
		for(j=0;j<COL_NUM;j++)//y direction
		{
			pointGroups.push_back(Vec3(((double)i)*c_max/((double)(ROW_NUM-1)),((double)j)*b_max/((double)(COL_NUM-1)),0.0 ));
			pointgroup_num++;	
			tex_p.push_back(Vec3( -((double)i)/((double)(ROW_NUM-1)),-((double)j)/((double)(COL_NUM-1)),0.0));
		}
	}
	v_point[0]=0;
	v_point[1]=COL_NUM-1;
	v_point[2]=COL_NUM*ROW_NUM-1;
	v_point[3]=COL_NUM*(ROW_NUM-1);

	vector<int> q_index[4];	//index of the 4 points of the quadrangle
	int quadrangel_num=0;			//number of the quadrangle

	for(i=0;i<(ROW_NUM-1);i++)
	{
		for(j=0;j<(COL_NUM-1);j++)
		{
			//store the quadrangle
			q_index[0].push_back(i*COL_NUM+j);
			q_index[1].push_back(i*COL_NUM+(j+1));
			q_index[2].push_back((i+1)*COL_NUM+(j+1));
			q_index[3].push_back((i+1)*COL_NUM+j);
			quadrangel_num++;
		}
	}

	Vec3 itome=Vec3(0.3*c_max,0.5*b_max,0.0);//point which connects with the string

	//create the model from input
	k_b=b_max;
	k_c=c_max;
	k_AR=b_max/c_max;

	k_p_num=pointgroup_num;	//���_���i�[
	k_q_num=quadrangel_num;	//�l�p�`�v�f���i�[

	k_s=itome;		//���ڒ��S��荞��

	for(i=0;i<pointgroup_num;i++)
	{
		k_tex_cd.push_back(tex_p[i]);

		k_local_cd.push_back(pointGroups[i]);	//���[�J�����W�͌�ŏd�S��ɂȂ�

		k_design_cd.push_back(pointGroups[i]-k_s);//�f�U�C�����W�͎��ڒ��S����ɂ��Ă���
		k_global_cd.push_back(QVRotate(k_orientation,k_design_cd[i]));	//�O���[�o�����W
		k_global_cd[i]+=k_pos;
	}
	
	k_S=0.0; k_mass=0.0;	//init
	element.resize(quadrangel_num);	//�l�p�`�v�f����element�m��
	for(i=0;i<quadrangel_num;i++)
	{
		element[i].b=k_b;
		element[i].c=k_c;

		//inxdex of the 4 points of the quadrangle
		element[i].index[0]=(int)q_index[0][i];
		element[i].index[1]=(int)q_index[1][i];
		element[i].index[2]=(int)q_index[2][i];
		element[i].index[3]=(int)q_index[3][i];

		//�w�����̌������l�p�`�̖ʐς��v�Z
		element[i].S=Heron(k_local_cd[element[i].index[0]],
							k_local_cd[element[i].index[1]],
							k_local_cd[element[i].index[2]])
						+ Heron(k_local_cd[element[i].index[0]],
							k_local_cd[element[i].index[2]],
							k_local_cd[element[i].index[3]]);

		//mass of the quadrangle
		element[i].mass=element[i].S*KITE_RHO;

		Vec3 v1,v2;
		double dummy;
		//normal of the quadrangle
		v1=k_local_cd[element[i].index[1]]-k_local_cd[element[i].index[0]];
		v2=k_local_cd[element[i].index[2]]-k_local_cd[element[i].index[1]];
		element[i].normal=cross(v1,v2);	//�O��
		dummy=unitize(element[i].normal);	//normalization

		//center gravity of the quadrangle
		element[i].cg=((k_local_cd[element[i].index[0]]+
							k_local_cd[element[i].index[1]]+
							k_local_cd[element[i].index[2]]+
							k_local_cd[element[i].index[3]]))*0.25;
		
		//�l�p�`�̃��[�J�������e���\��
		double Ixx=0.0,Iyy=0.0,Izz=0.0;//�������[�����g��
		double Ixy=0.0,Ixz=0.0,Iyz=0.0;//�������

		Vec3 dif;
		//�l�p�`�v�f�̒�Ӓ���b�����߂�
		dif=k_local_cd[element[i].index[0]]-k_local_cd[element[i].index[1]];
		b_max=norm(dif);
		//�l�p�`�v�f�̍���c�����߂�
		dif=k_local_cd[element[i].index[0]]-k_local_cd[element[i].index[3]];
		c_max=norm(dif);

		//�������[�����g
		Ixx=0.0833333*KITE_RHO*b_max*c_max*c_max*c_max;
		Iyy=0.0833333*KITE_RHO*b_max*b_max*b_max*c_max;
		Izz=0.0833333*KITE_RHO*b_max*c_max*(b_max*b_max+c_max*c_max);

		element[i].local_inertia_mom.data[0]=Ixx;
		element[i].local_inertia_mom.data[1]=Iyy;
		element[i].local_inertia_mom.data[2]=Izz;

		//�������
		Ixy=-0.0625*KITE_RHO*b_max*b_max*c_max*c_max;
		Ixz=0.0;
		Iyz=0.0;
		
		element[i].local_inertia_pro.data[0]=Ixy;
		element[i].local_inertia_pro.data[1]=Ixz;
		element[i].local_inertia_pro.data[2]=Iyz;

		//calculate the area and mass of the kite
		k_S+=element[i].S;
		k_mass+=element[i].mass;
	}
	cout<<k_mass<<endl;
	//calc the center gravity of the kite
	Vec3 mom=Vec3();
	for(i=0;i<quadrangel_num;i++)
	{
		mom+=element[i].mass*element[i].cg;
	}
	double kite_mass_inv=1.0/k_mass;
	k_cg=mom*kite_mass_inv;//�d�S

	//�d�S��̍��W�ւƕϊ�
	for(i=0;i<quadrangel_num;i++)
	{
		element[i].cg-=k_cg;
	}
	k_s-=k_cg;//���ڒ��S��
	//���[�J�����W��
	for(i=0;i<pointgroup_num;i++)
	{
		k_local_cd[i]-=k_cg;
	}

	//�����e���\���쐬
	double Ixx,Iyy,Izz,Ixy,Ixz,Iyz;
	Ixx=0.0;Iyy=0.0;Izz=0.0;//�������[�����g��
	Ixy=0.0;Ixz=0.0;Iyz=0.0;//�������
	for(i=0;i<quadrangel_num;i++)
	{
		Ixx+=element[i].local_inertia_mom.data[0]+
			element[i].mass*
			(element[i].cg.data[1]*element[i].cg.data[1]+
			element[i].cg.data[2]*element[i].cg.data[2]);
		Iyy+=element[i].local_inertia_mom.data[1]+
			element[i].mass*
			(element[i].cg.data[0]*element[i].cg.data[0]+
			element[i].cg.data[2]*element[i].cg.data[2]);
		Izz+=element[i].local_inertia_mom.data[2]+
			element[i].mass*
			(element[i].cg.data[1]*element[i].cg.data[1]+
			element[i].cg.data[0]*element[i].cg.data[0]);

		Ixy+=element[i].local_inertia_pro.data[0]+
			element[i].mass*
			(element[i].cg.data[0]*element[i].cg.data[1]);
		Ixz+=element[i].local_inertia_pro.data[1]+
			element[i].mass*
			(element[i].cg.data[0]*element[i].cg.data[2]);
		Iyz+=element[i].local_inertia_pro.data[2]+
			element[i].mass*
			(element[i].cg.data[2]*element[i].cg.data[1]);
	}

	inertia.SetValue(Ixx,-Ixy,-Ixz,-Ixy,Iyy,-Iyz,-Ixz,-Iyz,Izz);//�����e���\���Ɋi�[
	inertia_inv=inertia.Inverse();//�t�s��

	k_glb_s_pos=QVRotate(k_orientation,k_s)+k_pos;

}
void KiteSimulation::create_model_yak(void)
{
}
void KiteSimulation::create_model_dia(void)
{
}

//�e�[�u���̎Q��
//calculate the resistance cofficience from the attack angle
double KiteSimulation::search_alpha_CD(double alpha,double AR)
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
//calculate the lift coefficient from the attack angle
double KiteSimulation::search_alpha_CL(double alpha,double AR)
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

//calc the center of the kite from the attack angle
double KiteSimulation::search_alpha_x(double alpha)		//��-x�e�[�u��(�}���p���畗�S�����߂�)
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

//step update
void KiteSimulation::step_simulation(double dt)
{
	int i= 0;
	for(i=0;i<2;i++)
	{
		Lift[0][i] = Vec3();
		Lift[1][i] = Vec3();
		Drag[0][i]=Vec3();
		Drag[1][i]=Vec3();
	}

	//set the wind
	set_wind(dt);
	//Vec3 mm = wind_vel;

	//calc �׏d��moment
	calc_loads(dt);
	//calc the translation
	k_pos += k_global_vel*dt + 0.5*k_frc*dt*dt/k_mass;	//update the pos
	k_global_vel +=k_frc*dt/k_mass;						//update the velcolity

	//calculate the rotation
	//calc the angel speed from the local coordinate 
	Vec3 I_omega=inertia*k_omega;
	k_omega+=inertia_inv*(k_mom-cross(k_omega,I_omega)-Vec3(DAMP*k_omega.data[0],DAMP*k_omega.data[1],DAMP_yaw*k_omega.data[2]))*dt;
	//�V����4�������v�Z
	k_orientation+=(k_orientation*k_omega)*(0.5*dt);
	//����4�����̐��K��
	k_orientation.normalize();

	k_glb_s_pos=QVRotate(k_orientation,k_s)+k_pos;	//�O���[�o���n�̎��ڈʒu�X�V

	//line pos
	calc_line_pos(dt);

	k_glb_s_pos=QVRotate(k_orientation,k_s)+k_pos;	//�O���[�o���n�̎��ڈʒu�X�V


	//calc the velocity form local coordinate
	//�܂����̈ړ����x�ƕ������瑊�Α��x�����߂Ă���
	k_local_vel=k_global_vel-Wind;								//���Α��x(�����x�x�N�g��-���x�N�g��)
	k_local_vel=QVRotate(k_orientation.inverse(),k_local_vel);	//���[�J���n�փR���o�[�g

	//get the euler angles
	Vec3 dummy=k_orientation.GetEulerAngles();
	k_euler_angles=dummy;

	//calc the pos to render
	calc_render_pos();

	//global
	G[1]=k_pos;
	T_string[1]=k_glb_s_pos;
	for(i=0;i<2;i++)
	{
		//�ʒu
		Lift[i][1]=QVRotate(k_orientation,Lift[i][1])+k_pos;
		Drag[i][1]=QVRotate(k_orientation,Drag[i][1])+k_pos;
		//��
		Lift[i][0]=QVRotate(k_orientation,Lift[i][0]);
		Drag[i][0]=QVRotate(k_orientation,Drag[i][0]);
	}
}

//�����_�����O�p���W�擾
void KiteSimulation::calc_render_pos(void)
{
	int i=0,j=0;

	//�����
	for(i=0;i<ROW_NUM;i++)
	{
		for(j=0;j<COL_NUM;j++)
		{
			k_design_cd[(i*COL_NUM)+j].data[2]=p_l[1]-point[j].data[1];
		}
	}

	for(i=0;i<k_p_num;i++)
	{
		k_global_cd[i]=QVRotate(k_orientation,k_design_cd[i]);	//rotation
		k_global_cd[i]+=k_glb_s_pos;								//translation
	}
	
}

//set the wind
void KiteSimulation::set_wind(double dt)
{
	#define WIND_STR 6.0//*fabs(sin(dt*StepNo))

	int i,j,k;	//counter
	int N=GRID;	//number of the grid
	double h;	//�O���b�h��

	h = (l_init+1.0)*2.0/(double)N;

	double x,y,z;
	double ef=1.0;

	Vec3 kite_pos=Vec3(-k_pos.data[1],k_pos.data[2],k_pos.data[0]);

	for(i=1;i<=N;i++){//x���W
		if( -l_init+(i-1)*h<kite_pos.data[0] && kite_pos.data[0]<=-l_init+i*h ){
			for(j=1;j<=N;j++){//y���W
				if( -l_init+(j-1)*h<kite_pos.data[1] && kite_pos.data[1]<=-l_init+j*h ){
					for(k=1;k<=N;k++){//z���W
						if( -l_init+(k-1)*h<kite_pos.data[2] && kite_pos.data[2]<=-l_init+k*h ){
							
							x= g_w[IX(i,j,k)];
							y=-g_u[IX(i,j,k)];
							z= g_v[IX(i,j,k)];

							Wind=ef*l_init*1.0*Vec3(x,y,z);//�C���x�N�g���Z�b�g
						}
					}
				}
			}
		}
	}

}

//�׏d�ƃ��[�����g�̌v�Z
void KiteSimulation::calc_loads(double dt)
{
	//init the frc and moment
	k_frc=Vec3();
	k_mom=Vec3();

	int i=0;	//counter

	Vec3 Fb,Mb;
	Fb=Vec3();	//store the resultant force
	Mb=Vec3();	//���[�����g�̘a���i�[����

	double Fb_nrm=0.0;//normal of the resultant force

	//�F�X�Ȓl���i�[����
	double tmp=0.0;
	Vec3 tmp_vec=Vec3();

	Vec3 local_vel=Vec3();	//local speed
	Vec3 xz_vel=Vec3();		//x-z speed
	double xz_speed=0.0;	//speed
	Vec3 yz_vel=Vec3();		//y-z���ʂɂ����鑬�x
	double yz_speed=0.0;	//���x�̑傫��

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

	for(i=0;i<k_q_num;i++)
	{
		//�l�p�`�v�f�̑��x���v�Z
		tmp_vec=cross(k_omega,element[i].cg);	//�p���x����
		local_vel=k_local_vel+tmp_vec;				//���[�J���n�ɂ����鑬�x

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
		Lift_vec=cross(Drag_vec,-element[i].normal);
		Lift_vec=cross(Lift_vec,Drag_vec);
		tmp=unitize(Lift_vec);//���K��

		//�}���p�𒲂ׂ�
		tmp=dot(Drag_vec,element[i].normal);//cos(alpha)�����߂�
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
		cw=search_alpha_x(alpha);//�W��
		CD=search_alpha_CD(alpha,k_AR);
		CL=search_alpha_CL(alpha,k_AR);

		//�g�́C�R�͂��v�Z
		L=0.5*CL*RHO*(element[i].S*p_l[0])*(xz_speed*xz_speed)*Lift_vec;
		D=0.5*CD*RHO*(element[i].S*p_l[0])*(xz_speed*xz_speed)*Drag_vec;

		//���S�̂ł̍��͂��v�Z
		Fb+=L+D;

		Lift[0][0]+=L;
		Drag[0][0]+=D;

		//���S�̂ł̃��[�����g���v
		if(0.0<xz_vel.data[0])
		{
			cw=1.0-cw;
		}
		x_wind=Vec3((cw-0.5)*element[i].c+element[i].cg.data[0], element[i].cg.data[1], 0.0);

		tmp_vec=cross(x_wind,L);
		Mb+=tmp_vec;
		tmp_vec=cross(x_wind,D);
		Mb+=tmp_vec;

		Lift[0][1]=Vec3((cw)*element[i].c-k_cg.data[0],0.0,0.0);
		Drag[0][1]=Vec3((cw)*element[i].c-k_cg.data[0],0.0,0.0);

		//���̖͂@����������
		Fb_nrm+=dot((L+D),-element[i].normal);

//yz-------------------------------------------*/
		//�R�͂̉��������̎擾
		yz_speed=unitize(yz_vel);//���x�̑傫���̎擾�Ƒ��x�x�N�g���̐��K��
		Drag_vec=-yz_vel;

		//�g�͂̍�p��������̎擾
		Lift_vec=cross(Drag_vec,-element[i].normal);
		Lift_vec=cross(Lift_vec,Drag_vec);
		tmp=unitize(Lift_vec);//���K��

		//�}���p�𒲂ׂ�
		tmp=dot(Drag_vec,element[i].normal);//cos(psi)�����߂�
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
		cw=search_alpha_x(psi);//�W��
		CD=search_alpha_CD(psi,k_AR);
		CL=search_alpha_CL(psi,k_AR);

//#define PSI_SP 0.02734
//
//		cw+=PSI_SP;//alpha�p�̊֐���psi�ɗ��p���邽�߂̏C��

		//�g�́C�R�͂��v�Z
		L=0.5*CL*RHO*(element[i].S*p_l[0])*(yz_speed*yz_speed)*Lift_vec;
		D=0.5*CD*RHO*(element[i].S*p_l[0])*(yz_speed*yz_speed)*Drag_vec;

		//���S�̂ł̍��͂��v�Z
		Fb+=L+D;
		Lift[1][0]+=L;
		Drag[1][0]+=D;

		//���S�̂ł̃��[�����g���v
		if(0.0<yz_vel.data[1])
		{
			cw=1.0-cw;
		}
		x_wind=Vec3(element[i].cg.data[0], (cw-0.5)*element[i].b+element[i].cg.data[1], 0.0);

		kite_check=Vec3(0.0,(cw)*k_b-k_cg.data[1],0.0);
		tmp_vec=cross(x_wind,L);
		Mb+=tmp_vec;
		tmp_vec=cross(x_wind,D);
		Mb+=tmp_vec;

		Lift[1][1]=Vec3(0.0,(cw)*k_b-k_cg.data[1],0.0);
		Drag[1][1]=Vec3(0.0,(cw)*k_b-k_cg.data[1],0.0);

		//���̖͂@����������
		Fb_nrm+=dot((L+D),-element[i].normal);

//---------------------------------------------*/
	}
	kite_check=QVRotate(k_orientation,kite_check);
	
	//����݌v�Z��蓊�e�ʐ�(p_l)�����߂�
	calc_deflection(Fb_nrm);

	//�׏d�����[�J���n����O���[�o���n�ւƃR���o�[�g
	k_frc=QVRotate(k_orientation,Fb);

	//���[�����g
	k_mom+=Mb;

	//�d��(�O���[�o���n�ŉ�����)
	Vec3 g_vec=Vec3(0.0,0.0,1.0);
	g_vec*=k_mass*G_ACCE;
	k_frc+=g_vec;

	G[0]=g_vec;//�O���[�o��

	//1step�O�̒��͂����͂ɉ��Z
	k_frc+=k_T_spring[0];
	T_string[0]=k_T_spring[0];

	//���͂ɂ�郂�[�����g
	k_T_spring[0]=QVRotate(k_orientation.inverse(),k_T_spring[0]);//�O���[�o��->���[�J��
	Mb=cross(k_s,k_T_spring[0]);//���͂ɂ�郂�[�����g
	k_mom+=Mb;
}
//useless now
//calc the force from user
Vec3  KiteSimulation::calc_UI_force(void)
{
	Vec3 UI_vec=Vec3();
	double ce=0.03;

	UI_vec.data[0]=spring_ce[0]*ce;
	UI_vec.data[1]=-spring_ce[2]*ce;
	UI_vec.data[2]=spring_ce[1]*ce;

	return UI_vec;
}

//calc the position
void  KiteSimulation::calc_line_pos(double dt)//��(���̎��R�[�����̈ʒu�ɑΉ�)
{
	//calculate wind speed
	int line_e =  0.2;
	double wind_norm=0.0;
	Vec3 wind_vel=Wind;
	
	wind_norm=unitize(wind_vel);
	Vec3 w_s= wind_vel*(wind_norm*wind_norm)*0.02;

	double Inv_dt=1.0/dt;
	int i;
	//���ڈʒu�Ƃ̃����N

	line.setEndPoint(k_glb_s_pos);

	//kite.line_pos[0]+=kite3d::calc_UI_force()*dt;
	//k_T_spring[0]	= line.calTension();			//�΂˂ɂ�������
	//k_T_spring[0]	= Vec3(-2.7,0,1);			//�΂˂ɂ�������
	//init the windspeed to the line
	line.setWindSpeed(w_s);

	// update the line
	line.line_update();

	//���ڈʒu�Ƃ̃����N
	k_glb_s_pos = line.getEndPoint();
	k_pos=k_glb_s_pos-QVRotate(k_orientation,k_s);
	k_global_vel=(k_pos-k_pos_pre)*5;
	k_pos_pre=k_pos;

//	k_l_check=norm((kite.line_pos[LINE_SP]-kite.line_pos[0]));

}

//�t�@�C���ǂݍ���
int   KiteSimulation::read_file(const string &file_name, vector<double> &datas)
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
void  KiteSimulation::read_file(const string &file_name)
{
		vector<double> datas;
	int i;

	int number=read_file(file_name, datas);

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


//draw
void  KiteSimulation::draw_kite(void)	//draw the kite
{
	// draw line
	glPushMatrix();
		line.draw_Line();
	glPopMatrix();
		//draw teapot
	//glPushMatrix();
	//	static const GLfloat difg[] = { 0.4f, 0.6f, 0.4f, 1.0f };	// �g�U�F : ��
	//		glMaterialfv(GL_FRONT, GL_DIFFUSE, difg);
	//		glTranslatef(line.getEndPoint().data[0],line.getEndPoint().data[1],line.getEndPoint().data[2]);
	//		glutSolidTeapot(0.1);									//draw sphere
	//glPopMatrix();

	//draw kite shape
	int i=0,j=0,k=0;

	for(i=0;i<k_q_num;i++)
	{
		//glBegin(GL_QUADS);
		glBegin(GL_QUADS);

		//glNormal3d(-kite.element[i].normal[1],kite.element[i].normal[2],kite.element[i].normal[0]);
		glTexCoord2d(k_tex_cd[element[i].index[0]][1], k_tex_cd[element[i].index[0]][0]);
		glVertex3d(k_global_cd[element[i].index[0]][0], k_global_cd[element[i].index[0]][2], -k_global_cd[element[i].index[0]][1]);

		//glNormal3d(-kite.element[i].normal[1],kite.element[i].normal[2],kite.element[i].normal[0]);
		glTexCoord2d(k_tex_cd[element[i].index[1]][1], k_tex_cd[element[i].index[1]][0]);
		glVertex3d(k_global_cd[element[i].index[1]][0], k_global_cd[element[i].index[1]][2], -k_global_cd[element[i].index[1]][1]);

		//glNormal3d(-kite.element[i].normal[1],kite.element[i].normal[2],kite.element[i].normal[0]);
		glTexCoord2d(k_tex_cd[element[i].index[2]][1], k_tex_cd[element[i].index[2]][0]);
		glVertex3d(k_global_cd[element[i].index[2]][0], k_global_cd[element[i].index[2]][2], -k_global_cd[element[i].index[2]][1]);

		//glNormal3d(-kite.element[i].normal[1],kite.element[i].normal[2],kite.element[i].normal[0]);
		glTexCoord2d(k_tex_cd[element[i].index[3]][1], k_tex_cd[element[i].index[3]][0]);
		glVertex3d(k_global_cd[element[i].index[3]][0], k_global_cd[element[i].index[3]][2], -k_global_cd[element[i].index[3]][1]);

		glEnd();
	}
}

void  KiteSimulation::read_table(void)					//.dat����e�[�u����ǂݍ���
{
}



void  KiteSimulation::draw_options_01(void)	//�v���O�����m�F�p�I�v�V����
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

	glColor3f ( 0.0f, 0.0f, 1.0f );//blue
	glPushMatrix();
	//����
		glTranslated(T_string[1].data[0],T_string[1].data[2],-T_string[1].data[1]);
		glutSolidSphere(0.03,15,15);
	glPopMatrix();

	glEnable(GL_LIGHTING);
}
void  KiteSimulation::draw_options_02(void)	//�v���O�����m�F�p�I�v�V����
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

	//����
	glColor3f ( 1.0f, 0.5f, 0.0f );//orange
	glBegin ( GL_LINES );
		glVertex3d ( T_string[1].data[0],T_string[1].data[2],-T_string[1].data[1]);
		glVertex3d ( T_string[1].data[0]+scale*T_string[0].data[0],T_string[1].data[2]+scale*T_string[0].data[2],-(T_string[1].data[1]+scale*T_string[0].data[1]));
	glEnd ();

	glEnable(GL_LIGHTING);
}
void  KiteSimulation::draw_options_03(void)	//�v���O�����m�F�p�I�v�V����
{
		double scale=0.1;
	glDisable(GL_LIGHTING);

	glColor3f ( 1.0f, 0.0f, 0.0f );//red
	glBegin ( GL_LINES );
		glVertex3d ( G[1].data[0],G[1].data[2],-G[1].data[1] );
		glVertex3d ( G[1].data[0]+scale*k_frc.data[0],G[1].data[2]+scale*k_frc.data[2],-(G[1].data[1]+scale*k_frc.data[1]) );
	glEnd ();

	glEnable(GL_LIGHTING);
}

