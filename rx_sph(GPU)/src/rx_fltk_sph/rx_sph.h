/*!
  @file rx_sph.h
	
  @brief SPH�@
 
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


//! SPH�V�[���̃p�����[�^
struct rxSPHEnviroment
{
	int max_particles;			//!< �ő�p�[�e�B�N����
	Vec3 boundary_cen;			//!< ���E�̒��S
	Vec3 boundary_ext;			//!< ���E�̑傫��(�e�ӂ̒�����1/2)
	RXREAL dens;				//!< �������x
	RXREAL mass;				//!< �p�[�e�B�N���̎���
	RXREAL kernel_particles;	//!< �L�����ah�ȓ��̃p�[�e�B�N����
	RXREAL dt;					//!< ���ԃX�e�b�v��

	int use_inlet;				//!< �������E�����̗L��

	// �\�ʃ��b�V��
	Vec3 mesh_boundary_cen;		//!< ���b�V���������E�̒��S
	Vec3 mesh_boundary_ext;		//!< ���b�V���������E�̑傫��(�e�ӂ̒�����1/2)
	int mesh_vertex_store;		//!< ���_������|���S������\������Ƃ��̌W��
	int mesh_max_n;				//!< MC�@�p�O���b�h�̍ő啪����

	rxSPHEnviroment()
	{
		max_particles = 50000;
		boundary_cen = Vec3(0.0);
		boundary_ext = Vec3(2.0, 0.8, 0.8);
		dens = (RXREAL)998.29;
		mass = (RXREAL)0.04;
		kernel_particles = (RXREAL)20.0;
		dt = 0.01;

		mesh_vertex_store = 10;
		use_inlet = 0;
		mesh_max_n = 128;
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
// MARK:rxSPH�N���X�̐錾
//-----------------------------------------------------------------------------
class rxSPH : public rxParticleSystemBase
{
private:
	// �p�[�e�B�N��
	RXREAL *m_hNrm;					//!< �p�[�e�B�N���@��
	RXREAL *m_hFrc;					//!< �p�[�e�B�N���ɂ������
	RXREAL *m_hDens;				//!< �p�[�e�B�N�����x
	RXREAL *m_hPres;				//!< �p�[�e�B�N������

	// �\�ʐ����p(Anisotropic kernel)
	RXREAL *m_hUpPos;				//!< �������p�[�e�B�N���ʒu
	RXREAL *m_hPosW;				//!< �d�ݕt�����ύ��W
	RXREAL *m_hCMatrix;				//!< �����U�s��
	RXREAL *m_hEigen;				//!< �����U�s��̓��ْl
	RXREAL *m_hRMatrix;				//!< ��]�s��(�����U�s��̓��كx�N�g��)
	RXREAL *m_hG;					//!< �ό`�s��

	uint *m_hSurf;					//!< �\�ʃp�[�e�B�N��

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

	// ���q�p�����[�^
	uint m_iKernelParticles;		//!< �J�[�l�����̃p�[�e�B�N����
	RXREAL m_fInitDens, m_fMass;	//!< ���x�C����
	RXREAL m_fEffectiveRadius;		//!< �L�����a

	// �V�~�����[�V�����p�����[�^
	RXREAL m_fGasStiffness;			//!< �K�X�萔
	RXREAL m_fViscosity;			//!< �S���W��
	RXREAL m_fBuoyancy;				//!< ����

	// �J�[�l���֐��̌v�Z�̍ۂɗp������萔�W��
	double m_fWpoly6;				//!< Pory6�J�[�l���̒萔�W��
	double m_fGWpoly6;				//!< Pory6�J�[�l���̌��z�̒萔�W��
	double m_fLWpoly6;				//!< Pory6�J�[�l���̃��v���V�A���̒萔�W��
	double m_fWspiky;				//!< Spiky�J�[�l���̒萔�W��
	double m_fGWspiky;				//!< Spiky�J�[�l���̌��z�̒萔�W��
	double m_fLWspiky;				//!< Spiky�J�[�l���̃��v���V�A���̒萔�W��
	double m_fWvisc;				//!< Viscosity�J�[�l���̒萔�W��
	double m_fGWvisc;				//!< Viscosity�J�[�l���̌��z�̒萔�W��
	double m_fLWvisc;				//!< Viscosity�J�[�l���̃��v���V�A���̒萔�W��

protected:
	rxSPH(){}

public:
	//! �R���X�g���N�^
	rxSPH(bool use_opengl);

	//! �f�X�g���N�^
	virtual ~rxSPH();

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
	virtual void SetPolygonObstacle(const vector<Vec3> &vrts, const vector<Vec3> &nrms, const vector< vector<int> > &tris);
	virtual void SetBoxObstacle(Vec3 cen, Vec3 ext, Vec3 ang, int flg);
	virtual void SetSphereObstacle(Vec3 cen, double rad, int flg);
	virtual void MoveSphereObstacle(int b, Vec3 disp);
	virtual Vec3 GetSphereObstaclePos(int b = -1);

	// �z�X�g<->VBO�ԓ]��
	virtual RXREAL* GetArrayVBO(rxParticleArray type, bool d2h = true);
	virtual void SetArrayVBO(rxParticleArray type, const RXREAL* data, int start, int count);
	virtual void SetColorVBO(int type);

	// �A�֐��l�v�Z
	virtual double GetImplicit(double x, double y, double z);
	virtual void CalImplicitField(int n[3], Vec3 minp, Vec3 d, RXREAL *hF);
	virtual void CalImplicitFieldDevice(int n[3], Vec3 minp, Vec3 d, RXREAL *dF);

	// SPH���o��
	virtual void OutputSetting(string fn);

	// �`��֐�
	virtual void DrawCell(int i, int j, int k);
	virtual void DrawCells(Vec3 col, Vec3 col2, int sel = 0);
	virtual void DrawObstacles(void);


public:
	// SPH������
	void Initialize(const rxSPHEnviroment &env);
	void Allocate(int max_particles);
	void Finalize(void);

	// �ߖT�擾
	void GetNearestNeighbors(Vec3 pos, vector<rxNeigh> &neighs, RXREAL h = -1.0);
	void GetNearestNeighbors(int idx, RXREAL *p, vector<rxNeigh> &neighs, RXREAL h = -1.0);

	// �����Z���Ƀp�[�e�B�N�����i�[
	virtual void SetParticlesToCell(void);
	virtual void SetParticlesToCell(RXREAL *prts, int n, RXREAL h);

	// ���^�{�[���ɂ��A�֐��l
	double CalColorField(double x, double y, double z);

	// �����Z���Ƀ|���S�����i�[
	virtual void SetPolygonsToCell(void);

	int  GetPolygonsInCell(int gi, int gj, int gk, vector<int> &polys);
	bool IsPolygonsInCell(int gi, int gj, int gk);

	
	// �\�ʃp�[�e�B�N�����o
	void DetectSurfaceParticles(void);					// �\�ʃp�[�e�B�N���̌��o
	double CalDistToNormalizedMassCenter(const int i);	// �ߖT�p�[�e�B�N���̏d�S�܂ł̋����v�Z
	uint* GetArraySurf(void);							// �\�ʃp�[�e�B�N�����̎擾

	// �\�ʃp�[�e�B�N�����̎擾
	int GetSurfaceParticles(const Vec3 pos, RXREAL h, vector<rxSurfaceParticle> &sp);

	// �@���v�Z
	void CalNormalFromDensity(void);
	void CalNormal(void);


	//
	// Anisotropic kernel
	//
	virtual void CalAnisotropicKernel(void);

protected:
	// CPU�ɂ��SPH�v�Z
	void calDensity(void);
	void calNormal(void);
	void calForce(void);

	// �ʒu�Ƒ��x�̍X�V(Leap-Frog)
	void integrate(const RXREAL *pos, const RXREAL *vel, const RXREAL *frc, 
				   RXREAL *pos_new, RXREAL *vel_new, RXREAL dt);

	// �Փ˔���
	int calCollisionPolygon(uint grid_hash, Vec3 &pos0, Vec3 &pos1, Vec3 &vel, RXREAL dt);
	int calCollisionSolid(Vec3 &pos0, Vec3 &pos1, Vec3 &vel, RXREAL dt);
};


//-----------------------------------------------------------------------------
// MARK:rxSPH_GPU�N���X�̐錾
//-----------------------------------------------------------------------------
class rxSPH_GPU : public rxParticleSystemBase
{
private:
	//
	// �����o�ϐ�(GPU�ϐ�)
	//
	RXREAL *m_dPos;			//!< �p�[�e�B�N���ʒu
	RXREAL *m_dVel;			//!< �p�[�e�B�N�����x
	RXREAL *m_dNrm;			//!< �p�[�e�B�N���@��
	RXREAL *m_dFrc;			//!< �p�[�e�B�N���ɂ������
	RXREAL *m_dDens;		//!< �p�[�e�B�N�����x
	RXREAL *m_dPres;		//!< �p�[�e�B�N������

	uint *m_dAttr;			//!< �p�[�e�B�N������

	cudaGraphicsResource *m_pPosResource;	//!< OpenGL(��VBO)-CUDA�Ԃ̃f�[�^�]�����������߂̃n���h��


	// �\�ʐ����p(Anisotropic kernel)
	RXREAL *m_dUpPos;		//!< �������p�[�e�B�N���ʒu
	RXREAL *m_dPosW;		//!< �d�ݕt�����ύ��W
	RXREAL *m_dCMatrix;		//!< �����U�s��
	RXREAL *m_dEigen;		//!< �����U�s��̓��ْl
	RXREAL *m_dRMatrix;		//!< ��]�s��(�����U�s��̓��كx�N�g��)
	RXREAL *m_dG;			//!< �ό`�s��

	// �\�ʃ��b�V��
	RXREAL *m_dVrts;		//!< �ő̃��b�V�����_
	int    *m_dTris;		//!< �ő̃��b�V��

	// �V�~�����[�V�����p�����[�^
	rxSimParams m_params;	//!< �V�~�����[�V�����p�����[�^(GPU�ւ̃f�[�^�n���p)
	uint3 m_gridSize;		//!< �ߖT�T���O���b�h�̊e���̕�����
	uint m_numGridCells;	//!< �ߖT�T���O���b�h��������
	
	// ��ԕ���(GPU)
	rxParticleCell m_dCellData;	//!< �ߖT�T���O���b�h
	uint m_gridSortBits;		//!< �n�b�V���l�ɂ���\�[�g���̊����

	//
	// �����o�ϐ�(CPU�ϐ�)
	//
	RXREAL *m_hNrm;			//!< �p�[�e�B�N���@��
	RXREAL *m_hFrc;			//!< �p�[�e�B�N���ɂ������
	RXREAL *m_hDens;		//!< �p�[�e�B�N�����x
	RXREAL *m_hPres;		//!< �p�[�e�B�N������

	uint *m_hSurf;			//!< �\�ʃp�[�e�B�N��

	// �\�ʐ����p(Anisotropic kernel)
	RXREAL *m_hUpPos;		//!< �������p�[�e�B�N���ʒu
	RXREAL *m_hPosW;		//!< �d�ݕt�����ύ��W
	RXREAL *m_hCMatrix;		//!< �����U�s��
	RXREAL *m_hEigen;		//!< �����U�s��̓��ْl
	RXREAL *m_hRMatrix;		//!< ��]�s��(�����U�s��̓��كx�N�g��)
	RXREAL *m_hG;			//!< �ό`�s��
	RXREAL  m_fEigenMax;	//!< ���ْl�̍ő�l(�T�����a�g���ɗp����)

	// �\�ʃ��b�V��
	vector<RXREAL> m_vVrts;	//!< �ő̃��b�V�����_
	int m_iNumVrts;			//!< �ő̃��b�V�����_��
	vector<int> m_vTris;	//!< �ő̃��b�V��
	int m_iNumTris;			//!< �ő̃��b�V����

	bool m_bCalNormal;		//!< �@���v�Z�t���O

protected:
	rxSPH_GPU(){}

public:
	//! �R���X�g���N�^
	rxSPH_GPU(bool use_opengl);

	//! �f�X�g���N�^
	~rxSPH_GPU();

	// �p�[�e�B�N�����a
	float GetEffectiveRadius(){ return m_params.EffectiveRadius; }

	// �ߖT�p�[�e�B�N��
	uint* GetNeighborList(const int &i, int &n);

	// �V�~�����[�V�����p�����[�^
	rxSimParams GetParams(void){ return m_params; }
	void UpdateParams(void);

	// �t���O�ؑ�
	void ToggleNormalCalc(int t = -1){ RX_TOGGLE(m_bCalNormal, t); }			//!< �p�[�e�B�N���@���̌v�Z
	bool IsNormalCalc(void) const { return m_bCalNormal; }
#if MAX_BOX_NUM
	void ToggleSolidFlg(int t = -1);
#endif

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
	virtual void SetPolygonObstacle(const vector<Vec3> &vrts, const vector<Vec3> &nrms, const vector< vector<int> > &tris);
	virtual void SetBoxObstacle(Vec3 cen, Vec3 ext, Vec3 ang, int flg);
	virtual void SetSphereObstacle(Vec3 cen, double rad, int flg);
	virtual void MoveSphereObstacle(int b, Vec3 disp);
	virtual Vec3 GetSphereObstaclePos(int b = -1);

	// �z�X�g<->VBO�ԓ]��
	virtual RXREAL* GetArrayVBO(rxParticleArray type, bool d2h = true);
	virtual void SetArrayVBO(rxParticleArray type, const RXREAL* data, int start, int count);
	virtual void SetColorVBO(int type);


	// �A�֐��l�v�Z
	virtual double GetImplicit(double x, double y, double z);
	virtual void CalImplicitField(int n[3], Vec3 minp, Vec3 d, RXREAL *hF);
	virtual void CalImplicitFieldDevice(int n[3], Vec3 minp, Vec3 d, RXREAL *dF);

	// SPH���o��
	virtual void OutputSetting(string fn);

	// �`��֐�
	virtual void DrawCell(int i, int j, int k);
	virtual void DrawCells(Vec3 col, Vec3 col2, int sel = 0);
	virtual void DrawObstacles(void);

protected:
	void setObjectToCell(RXREAL *p);


public:
	// SPH������
	void Initialize(const rxSPHEnviroment &env);
	void Allocate(int max_particles);
	void Finalize(void);

	// �����Z���Ƀp�[�e�B�N�����i�[
	virtual void SetParticlesToCell(void);
	virtual void SetParticlesToCell(RXREAL *prts, int n, RXREAL h);
	virtual void SetPolygonsToCell(void);

	int  GetPolygonsInCell(int gi, int gj, int gk, vector<int> &polys){ return 0; }
	bool IsPolygonsInCell(int gi, int gj, int gk){ return false; }

	void CalMaxDensity(int k);

	// �\�ʃp�[�e�B�N�����o
	void DetectSurfaceParticles(void);					// �\�ʃp�[�e�B�N���̌��o
	double CalDistToNormalizedMassCenter(const int i);	// �ߖT�p�[�e�B�N���̏d�S�܂ł̋����v�Z
	uint* GetArraySurf(void);							// �\�ʃp�[�e�B�N�����̎擾

	// �\�ʃp�[�e�B�N�����̎擾
	int GetSurfaceParticles(const Vec3 pos, RXREAL h, vector<rxSurfaceParticle> &sp);

	// �@���v�Z
	void CalNormalFromDensity(void);
	void CalNormal(void);
	
	//
	// Anisotropic kernel
	//
	virtual void CalAnisotropicKernel(void);

protected:
	// �����Z���̏����ݒ�
	void setupCells(const Vec3 &vMin, const Vec3 &vMax, const double &h);

	// �O���b�h�n�b�V���̌v�Z
	uint calGridHash(uint x, uint y, uint z);
	uint calGridHash(Vec3 pos);

	// �|���S���𕪊��Z���Ɋi�[
	void setPolysToCell(RXREAL *vrts, int nv, int* tris, int nt);
};





#endif	// _SPH_H_

