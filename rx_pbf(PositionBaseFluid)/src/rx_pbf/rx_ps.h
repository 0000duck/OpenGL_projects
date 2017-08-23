/*!
  @file rx_ps.h
	
  @brief �p�[�e�B�N���������V�~�����[�V�����̊��N���X
 
  @author Makoto Fujisawa
  @date 2011-06
*/
// FILE --rx_ps.h--

#ifndef _RX_PS_H_
#define _RX_PS_H_


//-----------------------------------------------------------------------------
// MARK:�C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
#include "rx_sph_commons.h"

#include "rx_cu_common.cuh"

#include "rx_sph_solid.h"

//#include <helper_functions.h>



//-----------------------------------------------------------------------------
// ��`
//-----------------------------------------------------------------------------
#ifndef DIM
	#define DIM 4
#endif

const int RX_MAX_STEPS = 10000;


//-----------------------------------------------------------------------------
// �p�[�e�B�N���������C��
//-----------------------------------------------------------------------------
struct rxInletLine
{
	Vec3 pos1, pos2;	//!< ���C���̒[�_
	Vec3 vel;			//!< �ǉ�����p�[�e�B�N���̑��x
	Vec3 up;			//!< �p�[�e�B�N���͐ϕ���
	int accum;			//!< �p�[�e�B�N���͐ϐ�
	int span;			//!< ���ԓI�ȃX�p��
	double spacing;		//!< ��ԓI�ȃX�p��
};


//-----------------------------------------------------------------------------
// �p�[�e�B�N���������V�~�����[�V�����̊��N���X
//-----------------------------------------------------------------------------
class rxParticleSystemBase
{
public:

	enum rxParticleConfig
	{
		RX_CONFIG_RANDOM,
		RX_CONFIG_GRID,
		RX_CONFIG_BOX,
		RX_CONFIG_NONE, 
		_NUM_CONFIGS
	};

	enum rxParticleArray
	{
		RX_POSITION = 0,
		RX_VELOCITY,
		RX_FORCE, 
		RX_DENSITY, 

		RX_PREDICT_POS, 
		RX_PREDICT_VEL, 
		RX_SCALING_FACTOR, 
		RX_CORRECTION, 

		RX_BOUNDARY_PARTICLE, 
		RX_BOUNDARY_PARTICLE_VOL, 

		RX_TEST, 

		RX_CONSTANT, 
		RX_RAMP, 
		RX_NONE, 

		RX_PSDATA_END, 
	};

protected:
	bool m_bInitialized;
	bool m_bUseOpenGL;

	uint m_uNumParticles;	//!< ���݂̃p�[�e�B�N����
	uint m_uMaxParticles;	//!< �ő�p�[�e�B�N����

	uint m_uNumBParticles;	//!< ���E�p�[�e�B�N���̐�

	uint m_uNumArdGrid;

	RXREAL m_fParticleRadius;	//!< �p�[�e�B�N�����a(�L�����a�ł͂Ȃ�)
	Vec3   m_v3Gravity;			//!< �d��
	RXREAL m_fRestitution;		//!< �����W��[0,1]

	RXREAL *m_hPos;		//!< �p�[�e�B�N���ʒu
	RXREAL *m_hVel;		//!< �p�[�e�B�N�����x

	RXREAL *m_hPosB;	//!< ���E�p�[�e�B�N��
	RXREAL *m_hVolB;	//!< ���E�p�[�e�B�N���̑̐�

	RXREAL *m_hSb;		//!< ���E�p�[�e�B�N����Scaling factor

	uint m_posVBO;		//!< �p�[�e�B�N�����WVBO
	uint m_colorVBO;	//!< �J���[VBO

	RXREAL *m_hTmp;		//!< �ꎞ�I�Ȓl�̊i�[�ꏊ
	RXREAL m_fTmpMax;

	Vec3 m_v3EnvMin;	//!< ����AABB�ŏ����W
	Vec3 m_v3EnvMax;	//!< ����AABB�ő���W
	
	int m_iColorType;

	RXREAL m_fTime;

	vector<rxInletLine> m_vInletLines;	//!< �������C��
	int m_iInletStart;		//!< �p�[�e�B�N���ǉ��J�n�C���f�b�N�X

public:	
	vector<RXREAL> m_vFuncB;
	uint m_uNumBParticles0;	//!< �`�悵�Ȃ����E�p�[�e�B�N���̐�

protected:
	rxParticleSystemBase(){}

public:
	//! �R���X�g���N�^
	rxParticleSystemBase(bool bUseOpenGL) : 
		m_bInitialized(false),
		m_bUseOpenGL(bUseOpenGL), 
		m_hPos(0),
		m_hVel(0), 
		m_hTmp(0), 
		m_hPosB(0), 
		m_hVolB(0)
	{
		m_v3Gravity = Vec3(0.0, -9.82, 0.0);
		m_fRestitution = 0.0;
		m_fParticleRadius = 0.1;
		m_fTime = 0.0;
		m_iInletStart = -1;
		m_fTmpMax = 1.0;
		m_uNumBParticles0 = 0;
	}

	//! �f�X�g���N�^
	virtual ~rxParticleSystemBase(){}


	// �V�~�����[�V�������
	Vec3 GetMax(void) const { return m_v3EnvMax; }
	Vec3 GetMin(void) const { return m_v3EnvMin; }
	Vec3 GetDim(void) const { return m_v3EnvMax-m_v3EnvMin; }
	Vec3 GetCen(void) const { return 0.5*(m_v3EnvMax+m_v3EnvMin); }

	// �p�[�e�B�N����
	int	GetNumParticles() const { return m_uNumParticles; }
	int	GetMaxParticles() const { return m_uMaxParticles; }
	int GetNumBoundaryParticles() const { return m_uNumBParticles; }

	// �p�[�e�B�N�����a
	float GetParticleRadius(){ return m_fParticleRadius; }

	// �V�~�����[�V�����ݒ�
	void SetGravity(RXREAL x){ m_v3Gravity = Vec3(0.0, x, 0.0); }	//!< �d��

	// �p�[�e�B�N��VBO
	unsigned int GetCurrentReadBuffer(void) const { return m_posVBO; }
	unsigned int GetColorBuffer(void) const { return m_colorVBO; }

	// �`��p�J���[�ݒ�
	void SetColorType(int type){ m_iColorType = type; }
	int  GetColorType(void) const { return m_iColorType; }

public:
	// �������z�֐�
	virtual bool Update(RXREAL dt, int step = 0) = 0;

	virtual RXREAL* GetArrayVBO(rxParticleArray type, bool d2h = true, int num = -1) = 0;
	virtual void SetArrayVBO(rxParticleArray type, const RXREAL* data, int start, int count) = 0;
	virtual void SetColorVBO(int type, int picked) = 0;

	virtual RXREAL* GetParticle(void) = 0;
	virtual RXREAL* GetParticleDevice(void) = 0;

public:
	// ���z�֐�
	virtual void UnmapParticle(void){}

	virtual void SetPolygonObstacle(const vector<Vec3> &vrts, const vector<Vec3> &nrms, const vector< vector<int> > &tris, Vec3 vel){}
	virtual void SetPolygonObstacle(const string &filename, Vec3 cen, Vec3 ext, Vec3 ang, Vec3 vel){}
	virtual void SetBoxObstacle(Vec3 cen, Vec3 ext, Vec3 ang, Vec3 vel, int flg){}
	virtual void SetSphereObstacle(Vec3 cen, double rad, Vec3 vel, int flg){}

	virtual void SetParticlesToCell(void) = 0;
	virtual void SetParticlesToCell(RXREAL *prts, int n, RXREAL h) = 0;

	virtual void SetPolygonsToCell(void){}

	// �A�֐��l�v�Z
	virtual void CalImplicitField(int n[3], Vec3 minp, Vec3 d, RXREAL *hF){}
	virtual void CalImplicitFieldDevice(int n[3], Vec3 minp, Vec3 d, RXREAL *dF){}
	virtual double GetImplicit(double x, double y, double z){ return 0.0; }
	static RXREAL GetImplicit_s(double x, double y, double z, void* p)
	{
		return (RXREAL)(((rxParticleSystemBase*)p)->GetImplicit(x, y, z));
	}

	// �`��֐�
	virtual void DrawCell(int i, int j, int k){}
	virtual void DrawCells(Vec3 col, Vec3 col2, int sel = 0){}

	virtual void DrawObstacles(int drw){}

	// �V�~�����[�V�����ݒ�̏o��
	virtual void OutputSetting(string fn){}

	// ���q���̎擾
	virtual string GetParticleInfo(int i){ return "particle "+RX_TO_STRING(i); }

	// ���E�p�[�e�B�N���̏�����
	virtual void InitBoundary(void){}

public:
	void Reset(rxParticleConfig config);
	bool Set(const vector<Vec3> &ppos, const vector<Vec3> &pvel);

	void AddSphere(int start, RXREAL *pos, RXREAL *vel, int r, RXREAL spacing);
	void AddBox(int start, Vec3 cen, Vec3 dim, Vec3 vel, RXREAL spacing);
	int  AddLine(rxInletLine line);

	RXREAL SetColorVBOFromArray(RXREAL *hVal, int d, bool use_max = true, RXREAL vmax = 1.0);
	void SetColorVBO(int picked = -1){ SetColorVBO(m_iColorType, picked); }

	int OutputParticles(string fn);
	int InputParticles(string fn);

protected:
	int  addParticles(int &start, rxInletLine line);

	uint createVBO(uint size)
	{
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		return vbo;
	}

};



#endif	// _PS_H_

