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
const int DIM = 4;
const int RX_MAX_STEPS = 100000;


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
		RX_NORMAL, 
		RX_FORCE, 
		RX_DENSITY, 
		RX_PRESSURE, 

		RX_SURFACE, 
		RX_ATTRIBUTE, 

		RX_TEST, 

		RX_UPDATED_POSITION, 
		RX_EIGEN_VALUE, 
		RX_ROTATION_MATRIX, 
		RX_TRANSFORMATION, 
		RX_SUBPOSITION,

		RX_CONSTANT, 
		RX_RAMP, 
		RX_NONE, 

		RX_PSDATA_END, 
	};

protected:
	bool m_bInitialized;
	bool m_bUseOpenGL;

	uint m_uNumParticles;
	uint m_uMaxParticles;
	uint m_uNumArdGrid;

	uint m_solverIterations;

	RXREAL m_fParticleRadius;
	Vec3   m_v3Gravity;
	RXREAL m_fDamping;
	RXREAL m_fRestitution;

	RXREAL *m_hPos;		//!< �p�[�e�B�N���ʒu
	RXREAL *m_hVel;		//!< �p�[�e�B�N�����x

	uint *m_hAttr;		//!< �p�[�e�B�N������

	uint m_posVBO;		//!< �p�[�e�B�N�����WVBO
	uint m_colorVBO;	//!< �J���[VBO

	RXREAL *m_hTmp;		//!< �ꎞ�I�Ȓl�̊i�[�ꏊ
	RXREAL m_fTmpMax;
	
	Vec3 m_v3EnvMin;	//!< ����AABB�ŏ����W
	Vec3 m_v3EnvMax;	//!< ����AABB�ő���W

	int m_iColorType;

	RXREAL m_fTime;

	bool m_bCalNormal;		//!< �@���v�Z�t���O

	vector<rxInletLine> m_vInletLines;	//!< �������C��
	int m_iInletStart;		//!< �p�[�e�B�N���ǉ��J�n�C���f�b�N�X


protected:
	rxParticleSystemBase(){}

public:
	//! �R���X�g���N�^
	rxParticleSystemBase(bool bUseOpenGL) : 
		m_bInitialized(false),
		m_bUseOpenGL(bUseOpenGL), 
		m_hPos(0),
		m_hVel(0), 
		m_hAttr(0), 
		m_hTmp(0)
	{
		m_v3Gravity = Vec3(0.0, -9.82, 0.0);
		m_fDamping = 0.0;
		m_fRestitution = 0.0;
		m_fParticleRadius = 0.1;
		m_fTime = 0.0;
		m_bCalAnisotropic = false;
		m_iInletStart = -1;
		m_fTmpMax = 1.0;
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

	// �V�~�����[�V����������
	void SetIterations(int i) { m_solverIterations = i; }
		
	// �p�[�e�B�N�����a
	float GetParticleRadius(){ return m_fParticleRadius; }

	// �V�~�����[�V�����ݒ�
	void SetDamping(RXREAL x){ m_fDamping = x; }	//!< �ő̋��E�ł̔���
	void SetGravity(RXREAL x){ m_v3Gravity = Vec3(0.0, x, 0.0); }	//!< �d��

	// �p�[�e�B�N��VBO
	unsigned int GetCurrentReadBuffer() const { return m_posVBO; }
	unsigned int GetColorBuffer()	   const { return m_colorVBO; }

	// �`��p�J���[�ݒ�
	void SetColorType(int type){ m_iColorType = type; }
	int  GetColorType(void) const { return m_iColorType; }

	// �t���O�ؑ�
	void ToggleNormalCalc(int t = -1){ RX_TOGGLE(m_bCalNormal, t); }		//!< �p�[�e�B�N���@���̌v�Z
	bool IsNormalCalc(void) const { return m_bCalNormal; }

public:
	// �������z�֐�
	virtual bool Update(RXREAL dt, int step = 0) = 0;

	virtual RXREAL* GetArrayVBO(rxParticleArray type, bool d2h = true) = 0;
	virtual void SetArrayVBO(rxParticleArray type, const RXREAL* data, int start, int count) = 0;
	virtual void SetColorVBO(int type) = 0;

	virtual RXREAL* GetParticle(void) = 0;
	virtual RXREAL* GetParticleDevice(void) = 0;

public:
	// ���z�֐�
	virtual void UnmapParticle(void){}

	virtual void SetPolygonObstacle(const vector<Vec3> &vrts, const vector<Vec3> &nrms, const vector< vector<int> > &tris){}
	virtual void SetBoxObstacle(Vec3 cen, Vec3 ext, Vec3 ang, int flg){}
	virtual void SetSphereObstacle(Vec3 cen, double rad, int flg){}
	virtual void MoveSphereObstacle(int b, Vec3 disp){}
	virtual Vec3 GetSphereObstaclePos(int b = -1){ return Vec3(0.0); }

	virtual void SetParticlesToCell(void) = 0;
	virtual void SetParticlesToCell(RXREAL *prts, int n, RXREAL h) = 0;

	virtual void SetPolygonsToCell(void){}

	// �A�֐��l�v�Z
	virtual double GetImplicit(double x, double y, double z){ return 0.0; }
	virtual void CalImplicitField(int n[3], Vec3 minp, Vec3 d, RXREAL *hF){}
	virtual void CalImplicitFieldDevice(int n[3], Vec3 minp, Vec3 d, RXREAL *dF){}

	// �`��֐�
	virtual void DrawCell(int i, int j, int k){}
	virtual void DrawCells(Vec3 col, Vec3 col2, int sel = 0){}

	virtual void DrawObstacles(void){}

	// �V�~�����[�V�����ݒ�̏o��
	virtual void OutputSetting(string fn){}

	// Anisotropic Kernel
	virtual void CalAnisotropicKernel(void){}
	bool m_bCalAnisotropic;

public:
	void Reset(rxParticleConfig config);
	bool Set(const vector<Vec3> &ppos, const vector<Vec3> &pvel);

	void AddSphere(int start, RXREAL *pos, RXREAL *vel, int r, RXREAL spacing, int attr = 0);
	void AddBox(int start, Vec3 cen, Vec3 dim, Vec3 vel, RXREAL spacing, int attr = 0);
	int  AddLine(rxInletLine line);

	RXREAL SetColorVBOFromArray(RXREAL *hVal, int d, bool use_max = true, RXREAL vmax = 1.0);
	void SetColorVBO(void){ SetColorVBO(m_iColorType); }

	int OutputParticles(string fn);
	int InputParticles(string fn);

protected:
	int  addParticles(int &start, rxInletLine line, int attr = 0);

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

