/*!
  @file rx_sph.h
	
  @brief SPH�@
 
  @author Makoto Fujisawa
  @date 2008-10,2011-06
*/
// FILE --rx_sph.h--

#ifndef _RX_SPH_H_
#define _RX_SPH_H_


//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
#include "rx_sph_commons.h"

#include "rx_ps.h"			// �p�[�e�B�N���V�X�e�����N���X
#include "rx_nnsearch.h"	// �O���b�h�����ɂ��ߖT�T��

#include "rx_sph_solid.h"

#include "rx_kernel.h"

#include "rx_cu_common.cuh"



//-----------------------------------------------------------------------------
// ��`
//-----------------------------------------------------------------------------

//#define GL_REAL GL_DOUBLE
#define GL_REAL GL_FLOAT

// ���Ԍv��
class rxTimerAvg;
extern rxTimerAvg g_Time;

#define RX_USE_TIMER

#ifdef RX_USE_TIMER
#define RXTIMER_CLEAR g_Time.ClearTime()
#define RXTIMER_RESET g_Time.ResetTime()
#define RXTIMER(x) g_Time.Split(x)
#define RXTIMER_PRINT g_Time.Print()
#define RXTIMER_STRING(x) g_Time.PrintToString(x)
#else
#define RXTIMER_CLEAR
#define RXTIMER_RESET
#define RXTIMER(x) 
#define RXTIMER_PRINT
#define RXTIMER_STRING(x)
#endif

const RXREAL KOL2 = (RXREAL)0.561231024;	// 2^(-5/6)

// �O���[�o���ϐ��̐錾


inline RXREAL3 MAKE_RXREAL3(RXREAL x, RXREAL y, RXREAL z)
{
	return make_float3(x, y, z);
}
inline RXREAL2 MAKE_RXREAL2(RXREAL x, RXREAL y)
{
	return make_float2(x, y);
}
inline RXREAL3 MAKE_FLOAT3V(Vec3 x)
{
	return make_float3((FLOAT)x[0], (FLOAT)x[1], (FLOAT)x[2]);
}



//! SPH�V�[���̃p�����[�^
struct rxEnviroment
{
	#define MAX_DELETE_REGIONS 64

	int max_particles;			//!< �ő�p�[�e�B�N����
	Vec3 boundary_cen;			//!< ���E�̒��S
	Vec3 boundary_ext;			//!< ���E�̑傫��(�e�ӂ̒�����1/2)
	RXREAL dens;				//!< �������x
	RXREAL mass;				//!< �p�[�e�B�N���̎���
	RXREAL kernel_particles;	//!< �L�����ah�ȓ��̃p�[�e�B�N����
	RXREAL dt;					//!< ���ԃX�e�b�v��
	RXREAL viscosity;			//!< ���S���W��
	RXREAL gas_k;				//!< �K�X�萔

	int use_inlet;				//!< �������E�����̗L��

	RXREAL epsilon;				//!< CFM�̊ɘa�W��
	RXREAL eta;					//!< ���x�ϓ����e��
	int min_iter;				//!< ���R�r�����ŏ���
	int max_iter;				//!< ���R�r�����ő吔

	int use_ap;					//!< �l�H����ON/OFF (0 or 1)
	RXREAL ap_k;				//!< �l�H���͂̂��߂̌W��k (�{��)
	RXREAL ap_n;				//!< �l�H���͂̂��߂̌W��n (n��)
	RXREAL ap_q;				//!< �l�H���͌v�Z���̊�J�[�l���l�v�Z�p�W��(�L�����ah�ɑ΂���W��, [0,1])

	// �\�ʃ��b�V��
	Vec3 mesh_boundary_cen;		//!< ���b�V���������E�̒��S
	Vec3 mesh_boundary_ext;		//!< ���b�V���������E�̑傫��(�e�ӂ̒�����1/2)
	int mesh_vertex_store;		//!< ���_������|���S������\������Ƃ��̌W��
	int mesh_max_n;				//!< MC�@�p�O���b�h�̍ő啪����

	rxEnviroment()
	{
		max_particles = 50000;
		boundary_cen = Vec3(0.0);
		boundary_ext = Vec3(2.0, 0.8, 0.8);
		dens = (RXREAL)998.29;
		mass = (RXREAL)0.04;
		kernel_particles = (RXREAL)20.0;
		dt = 0.01;
		viscosity = 1.0e-3;
		gas_k = 3.0;

		mesh_vertex_store = 10;
		use_inlet = 0;
		mesh_max_n = 128;

		epsilon = 0.001;
		eta = 0.05;
		min_iter = 2;
		max_iter = 10;

		use_ap = true;
		ap_k = 0.1;
		ap_n = 4.0;
		ap_q = 0.2;
	}
};



//! �\�ʃp�[�e�B�N��
struct rxSurfaceParticle
{
	Vec3 pos;					//!< ���S���W
	Vec3 nrm;					//!< �@��
	Vec3 vel;					//!< ���x
	RXREAL d;					//!< �T�����S����̋���
	int idx;					//!< �p�[�e�B�N���C���f�b�N�X
};

extern double g_fSurfThr[2];


//-----------------------------------------------------------------------------
// MARK:rxPBDSPH�N���X�̐錾
//  - Miles Macklin and Matthias Muller, "Position Based Fluids", Proc. SIGGRAPH 2013, 2013. 
//  - http://blog.mmacklin.com/publications/
//-----------------------------------------------------------------------------
class rxPBDSPH : public rxParticleSystemBase
{
private:
	// �p�[�e�B�N��
	RXREAL *m_hFrc;					//!< �p�[�e�B�N���ɂ������
	RXREAL *m_hDens;				//!< �p�[�e�B�N�����x

	RXREAL *m_hS;					//!< Scaling factor for CFM
	RXREAL *m_hDp;					//!< �ʒu�C����

	RXREAL *m_hPredictPos;			//!< �\���ʒu
	RXREAL *m_hPredictVel;			//!< �\�����x

	RXREAL *m_hSb;					//!< ���E�p�[�e�B�N����Scaling factor

	// ���E�E�ő�
	rxSolid *m_pBoundary;			//!< �V�~�����[�V������Ԃ̋��E
	vector<rxSolid*> m_vSolids;		//!< �ő̕���
	RXREAL *m_hVrts;				//!< �ő̃|���S���̒��_
	int m_iNumVrts;					//!< �ő̃|���S���̒��_��
	int *m_hTris;					//!< �ő̃|���S��
	int m_iNumTris;					//!< �ő̃|���S���̐�

	// ��ԕ����i�q�֘A
	rxNNGrid *m_pNNGrid;			//!< �����O���b�h�ɂ��ߖT�T��
	vector< vector<rxNeigh> > m_vNeighs;	//!< �ߖT�p�[�e�B�N��

	rxNNGrid *m_pNNGridB;			//!< ���E�p�[�e�B�N���p�����O���b�h
	vector< vector<rxNeigh> > m_vNeighsB;	//!< ���E�ߖT�p�[�e�B�N��


	// ���q�p�����[�^
	uint m_iKernelParticles;		//!< �J�[�l�����̃p�[�e�B�N����
	RXREAL m_fRestDens, m_fMass;	//!< ���x�C����
	RXREAL m_fEffectiveRadius;		//!< �L�����a
	RXREAL m_fKernelRadius;			//!< �J�[�l���̉e���͈�

	RXREAL m_fRestConstraint;		//!< �X�P�[�����O�t�@�N�^�̕��ꍀ

	// �V�~�����[�V�����p�����[�^
	RXREAL m_fViscosity;			//!< �S���W��

	RXREAL m_fEpsilon;				//!< CFM�̊ɘa�W��
	RXREAL m_fEta;					//!< ���x�ϓ���
	int m_iMinIterations;			//!< ���R�r�����ŏ�������
	int m_iMaxIterations;			//!< ���R�r�����ő唽����

	bool m_bArtificialPressure;		//!< �N���X�^�����O��h�����߂�Artificial Pressure����ǉ�����t���O
	RXREAL m_fApK;					//!< �l�H���͂̂��߂̌W��k
	RXREAL m_fApN;					//!< �l�H���͂̂��߂̌W��n
	RXREAL m_fApQ;					//!< �l�H���͌v�Z���̊�J�[�l���l�v�Z�p�W��(�L�����ah�ɑ΂���W��, [0,1])


	// �J�[�l���֐��̌v�Z�̍ۂɗp������萔�W��
	double m_fAw;
	double m_fAg;
	double m_fAl;

	// �J�[�l���֐�
	double (*m_fpW)(double, double, double);
	Vec3 (*m_fpGW)(double, double, double, Vec3);
	double (*m_fpLW)(double, double, double, double);

protected:
	rxPBDSPH(){}

public:
	//! �R���X�g���N�^
	rxPBDSPH(bool use_opengl);

	//! �f�X�g���N�^
	virtual ~rxPBDSPH();

	// �p�[�e�B�N�����a
	float GetEffectiveRadius(){ return m_fEffectiveRadius; }

	// �ߖT�p�[�e�B�N��
	uint* GetNeighborList(const int &i, int &n);

public:
	//
	// ���z�֐�
	//
	// �p�[�e�B�N���f�[�^
	virtual RXREAL* GetParticle(void);
	virtual RXREAL* GetParticleDevice(void);
	virtual void UnmapParticle(void){}

	// �V�~�����[�V�����X�e�b�v
	virtual bool Update(RXREAL dt, int step = 0);

	// �V�[���̐ݒ�
	virtual void SetPolygonObstacle(const vector<Vec3> &vrts, const vector<Vec3> &nrms, const vector< vector<int> > &tris, Vec3 vel);
	virtual void SetBoxObstacle(Vec3 cen, Vec3 ext, Vec3 ang, Vec3 vel, int flg);
	virtual void SetSphereObstacle(Vec3 cen, double rad, Vec3 vel, int flg);

	// �z�X�g<->VBO�ԓ]��
	virtual RXREAL* GetArrayVBO(rxParticleArray type, bool d2h = true, int num = -1);
	virtual void SetArrayVBO(rxParticleArray type, const RXREAL* data, int start, int count);
	virtual void SetColorVBO(int type, int picked);

	// �A�֐��l�v�Z
	virtual double GetImplicit(double x, double y, double z);
	virtual void CalImplicitField(int n[3], Vec3 minp, Vec3 d, RXREAL *hF);
	virtual void CalImplicitFieldDevice(int n[3], Vec3 minp, Vec3 d, RXREAL *dF);

	// SPH���o��
	virtual void OutputSetting(string fn);

	// �`��֐�
	virtual void DrawCell(int i, int j, int k);
	virtual void DrawCells(Vec3 col, Vec3 col2, int sel = 0);
	virtual void DrawObstacles(int drw);


public:
	// SPH������
	void Initialize(const rxEnviroment &env);
	void Allocate(int max_particles);
	void Finalize(void);

	// �ߖT�擾
	void GetNearestNeighbors(Vec3 pos, vector<rxNeigh> &neighs, RXREAL h = -1.0);
	void GetNearestNeighbors(int idx, RXREAL *p, vector<rxNeigh> &neighs, RXREAL h = -1.0);

	void GetNearestNeighborsB(Vec3 pos, vector<rxNeigh> &neighs, RXREAL h = -1.0);

	// �����Z���Ƀp�[�e�B�N�����i�[
	virtual void SetParticlesToCell(void);
	virtual void SetParticlesToCell(RXREAL *prts, int n, RXREAL h);

	// ���^�{�[���ɂ��A�֐��l
	double CalColorField(double x, double y, double z);

	// �����Z���Ƀ|���S�����i�[
	virtual void SetPolygonsToCell(void);

	int  GetPolygonsInCell(int gi, int gj, int gk, set<int> &polys);
	bool IsPolygonsInCell(int gi, int gj, int gk);

	// �l�����͍�
	bool& GetArtificialPressure(void){ return m_bArtificialPressure; }


protected:
	// CPU�ɂ��SPH�v�Z
	void calDensity(const RXREAL *pos, RXREAL *dens, RXREAL h);
	void calForceExtAndVisc(const RXREAL *pos, const RXREAL *vel, const RXREAL *dens, RXREAL *frc, RXREAL h);

	// Scaling factor�̌v�Z
	void calScalingFactor(const RXREAL *ppos, RXREAL *pdens, RXREAL *pscl, RXREAL h, RXREAL dt);

	// ��T���ʂ̌v�Z
	void calPositionCorrection(const RXREAL *ppos, const RXREAL *pscl, RXREAL *pdp, RXREAL h, RXREAL dt);

	// rest density�̌v�Z
	RXREAL calRestDensity(RXREAL h);

	// ���E�p�[�e�B�N���̑̐ς��v�Z
	void calBoundaryVolumes(const RXREAL *bpos, RXREAL *bvol, RXREAL mass, uint n, RXREAL h);

	// ���ԃX�e�b�v���̏C��
	RXREAL calTimeStep(RXREAL &dt, RXREAL eta_avg, const RXREAL *pfrc, const RXREAL *pvel, const RXREAL *pdens);

	// �ʒu�Ƒ��x�̍X�V
	void integrate(const RXREAL *pos, const RXREAL *vel, const RXREAL *dens, const RXREAL *acc, 
				   RXREAL *pos_new, RXREAL *vel_new, RXREAL dt);
	void integrate2(const RXREAL *pos, const RXREAL *vel, const RXREAL *dens, const RXREAL *acc, 
				    RXREAL *pos_new, RXREAL *vel_new, RXREAL dt);

	// �Փ˔���
	int calCollisionPolygon(uint grid_hash, Vec3 &pos0, Vec3 &pos1, Vec3 &vel, RXREAL dt);
	int calCollisionSolid(Vec3 &pos0, Vec3 &pos1, Vec3 &vel, RXREAL dt);
};




//-----------------------------------------------------------------------------
// MARK:rxPBDSPH_GPU�N���X�̐錾
//  - Miles Macklin and Matthias Muller, "Position Based Fluids", Proc. SIGGRAPH 2013, 2013. 
//  - http://blog.mmacklin.com/publications/
//-----------------------------------------------------------------------------
class rxPBDSPH_GPU : public rxParticleSystemBase
{
private:
	//
	// �����o�ϐ�(GPU�ϐ�)
	//
	RXREAL *m_dPos;			//!< �p�[�e�B�N���ʒu
	RXREAL *m_dVel;			//!< �p�[�e�B�N�����x

	RXREAL *m_dFrc;			//!< �p�[�e�B�N���ɂ������
	RXREAL *m_dDens;		//!< �p�[�e�B�N�����x

	RXREAL *m_dPosB;		//!< ���E�p�[�e�B�N��
	RXREAL *m_dVolB;		//!< ���E�p�[�e�B�N���̑̐�

	RXREAL *m_dS;			//!< Scaling factor for CFM
	RXREAL *m_dDp;			//!< �ʒu�C����
	RXREAL *m_dPredictPos;	//!< �\���ʒu
	RXREAL *m_dPredictVel;	//!< �\�����x

	RXREAL *m_dSb;			//!< ���E�p�[�e�B�N����Scaling factor

	RXREAL *m_dErr;			//!< ���x�ϓ��l	
	RXREAL *m_dErrScan;		//!< ���x�ϓ��l��Scan����

	cudaGraphicsResource *m_pPosResource;	//!< OpenGL(��VBO)-CUDA�Ԃ̃f�[�^�]�����������߂̃n���h��

	// �\�ʃ��b�V��
	RXREAL *m_dVrts;		//!< �ő̃��b�V�����_
	int    *m_dTris;		//!< �ő̃��b�V��


	// �V�~�����[�V�����p�����[�^
	rxSimParams m_params;	//!< �V�~�����[�V�����p�����[�^(GPU�ւ̃f�[�^�n���p)
	uint3 m_gridSize;		//!< �ߖT�T���O���b�h�̊e���̕�����
	uint m_numGridCells;	//!< �ߖT�T���O���b�h��������
	
	// ��ԕ���(GPU)
	rxParticleCell m_dCellData;			//!< �ߖT�T���O���b�h
	uint m_gridSortBits;				//!< �n�b�V���l�ɂ���\�[�g���̊����

	rxParticleCell m_dCellDataB;		//!< ���E�p�[�e�B�N���p�ߖT�T���O���b�h
	uint3 m_gridSizeB;					//!< ���E�p�[�e�B�N���p�ߖT�T���O���b�h�̊e���̕�����

	//
	// �����o�ϐ�(CPU�ϐ�)
	//
	RXREAL *m_hFrc;			//!< �p�[�e�B�N���ɂ������
	RXREAL *m_hDens;		//!< �p�[�e�B�N�����x

	RXREAL *m_hS;			//!< Scaling factor for CFM
	RXREAL *m_hDp;			//!< �ʒu�C����
	RXREAL *m_hPredictPos;	//!< �\���ʒu
	RXREAL *m_hPredictVel;	//!< �\�����x

	RXREAL *m_hSb;			//!< ���E�p�[�e�B�N����Scaling factor

	// �\�ʃ��b�V��
	rxPolygons m_Poly;
	vector<RXREAL> m_vVrts;	//!< �ő̃��b�V�����_
	int m_iNumVrts;			//!< �ő̃��b�V�����_��
	vector<int> m_vTris;	//!< �ő̃��b�V��
	int m_iNumTris;			//!< �ő̃��b�V����

	RXREAL m_fEpsilon;				//!< CFM�̊ɘa�W��
	RXREAL m_fEta;					//!< ���x�ϓ���
	int m_iMinIterations;			//!< ���R�r�����ŏ�������
	int m_iMaxIterations;			//!< ���R�r�����ő唽����

	bool m_bArtificialPressure;		//!< �N���X�^�����O��h�����߂�Artificial Pressure����ǉ�����t���O

	vector<rxSolid*> m_vSolids;		//!< �ő̕���(�ő̃p�[�e�B�N�������p)

protected:
	rxPBDSPH_GPU(){}

public:
	//! �R���X�g���N�^
	rxPBDSPH_GPU(bool use_opengl);

	//! �f�X�g���N�^
	~rxPBDSPH_GPU();

	// �p�[�e�B�N�����a
	float GetEffectiveRadius(){ return m_params.EffectiveRadius; }

	// �ߖT�p�[�e�B�N��
	uint* GetNeighborList(const int &i, int &n);

	// �V�~�����[�V�����p�����[�^
	rxSimParams GetParams(void){ return m_params; }
	void UpdateParams(void);

public:
	//
	// ���z�֐�
	//
	// �p�[�e�B�N���f�[�^
	virtual RXREAL* GetParticle(void);
	virtual RXREAL* GetParticleDevice(void);
	virtual void UnmapParticle(void);

	// �V�~�����[�V�����X�e�b�v
	virtual bool Update(RXREAL dt, int step = 0);

	// �V�[���̐ݒ�
	virtual void SetPolygonObstacle(const vector<Vec3> &vrts, const vector<Vec3> &nrms, const vector< vector<int> > &tris, Vec3 vel);
	virtual void SetPolygonObstacle(const string &filename, Vec3 cen, Vec3 ext, Vec3 ang, Vec3 vel);
	virtual void SetBoxObstacle(Vec3 cen, Vec3 ext, Vec3 ang, Vec3 vel, int flg);
	virtual void SetSphereObstacle(Vec3 cen, double rad, Vec3 vel, int flg);

	// �z�X�g<->VBO�ԓ]��
	virtual RXREAL* GetArrayVBO(rxParticleArray type, bool d2h = true, int num = -1);
	virtual void SetArrayVBO(rxParticleArray type, const RXREAL* data, int start, int count);
	virtual void SetColorVBO(int type, int picked);


	// �A�֐��l�v�Z
	virtual double GetImplicit(double x, double y, double z);
	virtual void CalImplicitField(int n[3], Vec3 minp, Vec3 d, RXREAL *hF);
	virtual void CalImplicitFieldDevice(int n[3], Vec3 minp, Vec3 d, RXREAL *dF);

	// SPH���o��
	virtual void OutputSetting(string fn);

	// �`��֐�
	virtual void DrawCell(int i, int j, int k);
	virtual void DrawCells(Vec3 col, Vec3 col2, int sel = 0);
	virtual void DrawObstacles(int drw);

protected:
	void setObjectToCell(RXREAL *p, RXREAL *v);


public:
	// SPH������
	void Initialize(const rxEnviroment &env);
	void Allocate(int max_particles);
	void Finalize(void);

	// �����Z���Ƀp�[�e�B�N�����i�[
	virtual void SetParticlesToCell(void);
	virtual void SetParticlesToCell(RXREAL *prts, int n, RXREAL h);
	virtual void SetPolygonsToCell(void);

	int  GetPolygonsInCell(int gi, int gj, int gk, vector<int> &polys){ return 0; }
	bool IsPolygonsInCell(int gi, int gj, int gk){ return false; }

	void CalMaxDensity(int k);

	// �l�����͍�
	bool& GetArtificialPressure(void){ return m_bArtificialPressure; }

	// ���E�p�[�e�B�N���̏�����
	virtual void InitBoundary(void);


protected:
	// rest density�̌v�Z
	RXREAL calRestDensity(RXREAL h);

	// �����Z���̏����ݒ�
	void setupCells(rxParticleCell &cell, uint3 &gridsize, double &cell_width, Vec3 vMin, Vec3 vMax, double h);

	// �O���b�h�n�b�V���̌v�Z
	uint calGridHash(uint x, uint y, uint z);
	uint calGridHash(Vec3 pos);

	// �|���S���𕪊��Z���Ɋi�[
	void setPolysToCell(RXREAL *vrts, int nv, int* tris, int nt);

	// �|���S�����_�̒���
	bool fitVertices(const Vec3 &ctr, const Vec3 &sl, vector<Vec3> &vec_set);
};



#endif	// _SPH_H_

