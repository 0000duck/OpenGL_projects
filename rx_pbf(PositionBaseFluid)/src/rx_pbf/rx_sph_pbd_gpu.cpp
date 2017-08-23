/*!
  @file rx_sph_pbd.cpp
	
  @brief Position based fluid�̎���(GPU)
 
  @author Makoto Fujisawa
  @date   2013-06
*/


//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
#include "rx_sph.h"

#include "rx_cu_funcs.cuh"
#include <cuda_runtime.h>

#include "rx_pcube.h"



//-----------------------------------------------------------------------------
// �O���[�o���ϐ�
//-----------------------------------------------------------------------------
extern int g_iIterations;			//!< �C��������
extern double g_fEta;				//!< ���x�ϓ���


//-----------------------------------------------------------------------------
// rxPBDSPH_GPU�N���X�̎���
//-----------------------------------------------------------------------------
/*!
 * �R���X�g���N�^
 * @param[in] use_opengl VBO�g�p�t���O
 */
rxPBDSPH_GPU::rxPBDSPH_GPU(bool use_opengl) : 
	rxParticleSystemBase(use_opengl), 
	m_hFrc(0),
	m_hDens(0), 
	m_hPredictPos(0), 
	m_hPredictVel(0), 
	m_hS(0), 
	m_hDp(0), 
	m_hSb(0), 
	m_dPos(0),
	m_dVel(0),
	m_dFrc(0), 
	m_dDens(0), 
	m_dPredictPos(0), 
	m_dPredictVel(0), 
	m_dS(0), 
	m_dDp(0), 
	m_dSb(0), 
	m_dErr(0), 
	m_dErrScan(0), 
	m_dPosB(0), 
	m_dVolB(0),
	m_dVrts(0), 
	m_dTris(0)
{
	m_params.Gravity = make_float3(m_v3Gravity[0], m_v3Gravity[1], m_v3Gravity[2]);

	m_params.Dt = 0.01f;

	m_params.Viscosity = 0.01;

	m_params.Restitution = 0.0f;

	m_params.VorticityConfinement = 1.0f;
	m_params.Threshold = 1.0f;
	m_params.Buoyancy = 0.0f;

	m_fEpsilon = 0.001;
	m_bArtificialPressure = true;

	m_params.BoxNum = 0;
	m_params.SphereNum = 0;

	m_fRestitution = 0.0f;

	m_dCellData.dSortedPolyIdx = 0;
	m_dCellData.dGridPolyHash = 0;
	m_dCellData.dPolyCellStart = 0;
	m_dCellData.dPolyCellEnd = 0;
	m_dCellData.uNumPolyHash = 0;

	m_dCellDataB.dSortedPolyIdx = 0;
	m_dCellDataB.dGridPolyHash = 0;
	m_dCellDataB.dPolyCellStart = 0;
	m_dCellDataB.dPolyCellEnd = 0;
	m_dCellDataB.uNumPolyHash = 0;

	m_fTime = 0.0f;
	m_uNumParticles = 0;
	m_iNumVrts = 0;
	m_iNumTris = 0;

	m_uNumBParticles = 0;

	m_iColorType = RX_RAMP;
}

/*!
 * �f�X�g���N�^
 */
rxPBDSPH_GPU::~rxPBDSPH_GPU()
{
	Finalize();
	CuClearData();
}



/*!
 * �V�~�����[�V�����̏�����
 * @param[in] max_particles �ő�p�[�e�B�N����
 * @param[in] boundary_ext ���E�̑傫��(�e�ӂ̒�����1/2)
 * @param[in] dens �������x
 * @param[in] mass �p�[�e�B�N���̎���
 * @param[in] kernel_particle �L�����ah�ȓ��̃p�[�e�B�N����
 */
void rxPBDSPH_GPU::Initialize(const rxEnviroment &env)
{
	// MARK:Initialize
	RXCOUT << "[rxPBDSPH_GPU::Initialize]" << endl;

	m_params.Density = env.dens;
	m_params.Mass    = env.mass;
	m_params.KernelParticles = env.kernel_particles;

	m_params.Volume = env.kernel_particles*m_params.Mass/m_params.Density;

	m_params.EffectiveRadius = pow(((3.0*m_params.Volume)/(4.0*RX_PI)), 1.0/3.0);
	m_fParticleRadius = pow((RX_PI/(6.0*m_params.KernelParticles)), 1.0/3.0)*m_params.EffectiveRadius;
	//m_fParticleRadius = 0.5f*m_params.EffectiveRadius;
	m_params.ParticleRadius = m_fParticleRadius;

	m_params.Viscosity = env.viscosity;
	m_params.GasStiffness = env.gas_k;

	RXREAL h = m_params.EffectiveRadius;
	RXREAL r = m_fParticleRadius;

	// �J�[�l���֐��̒萔
	m_params.Wpoly6  =  315.0/(64.0*RX_PI*pow(h, (RXREAL)9.0));
	m_params.GWpoly6 = -945.0/(32.0*RX_PI*pow(h, (RXREAL)9.0));
	m_params.LWpoly6 = -945.0/(32.0*RX_PI*pow(h, (RXREAL)9.0));

	m_params.Wspiky  =  15.0/(RX_PI*pow(h, (RXREAL)6.0));
	m_params.GWspiky = -45.0/(RX_PI*pow(h, (RXREAL)6.0));
	m_params.LWspiky = -90.0/(RX_PI*pow(h, (RXREAL)6.0));

	m_params.Wvisc   = 15.0/(2.0*RX_PI*pow(h, (RXREAL)3.0));
	m_params.GWvisc  = 15.0/(2.0*RX_PI*pow(h, (RXREAL)3.0));
	m_params.LWvisc  = 45.0/(RX_PI*pow(h, (RXREAL)6.0));

	// �������x�̌v�Z
	m_params.Density = calRestDensity(h);

	// PBD�p�p�����[�^
	m_fEpsilon = env.epsilon;
	m_fEta = env.eta;
	m_iMinIterations = env.min_iter;
	m_iMaxIterations = env.max_iter;

	// �l�H���͂̂��߂̌W��
	m_bArtificialPressure = (env.use_ap ? true : false);
	m_params.AP = env.use_ap;
	m_params.AP_K = env.ap_k;
	m_params.AP_N = env.ap_n;
	m_params.AP_Q = env.ap_q;
	RXREAL dp = (env.ap_q)*h;
	m_params.AP_WQ = KernelPoly6(dp, h, m_params.Wpoly6);

	RXCOUT << "particle : " << endl;
	RXCOUT << " n_max = " << env.max_particles << endl;
	RXCOUT << " h = " << m_params.EffectiveRadius << endl;
	RXCOUT << " r = " << m_params.ParticleRadius << endl;
	RXCOUT << " dens = " << m_params.Density << endl;
	RXCOUT << " mass = " << m_params.Mass << endl;
	RXCOUT << " kernel_particles = " << m_params.KernelParticles << endl;
	RXCOUT << " volume = " << m_params.Volume << endl << endl;
	RXCOUT << " viscosity = " << m_params.Viscosity << endl;
	RXCOUT << " epsilon = " << m_fEpsilon << endl;
	RXCOUT << " eta = " << m_fEta << endl;
	RXCOUT << " min_iter = " << m_iMinIterations << endl;
	RXCOUT << " max_iter = " << m_iMaxIterations << endl;
	RXCOUT << " artificial pressure : " << (m_bArtificialPressure ? "on" : "off") << endl;
	RXCOUT << "  k = " << m_params.AP_K << endl;
	RXCOUT << "  n = " << m_params.AP_N << endl;
	RXCOUT << "  dq = " << m_params.AP_Q << endl;
	RXCOUT << "  wq = " << m_params.AP_WQ << endl;
	

	//
	// ���E�ݒ�
	//
	// ���E�͈̔�
	// �V�~�����[�V�������̑傫��
	m_v3EnvMin = env.boundary_cen-env.boundary_ext;
	m_v3EnvMax = env.boundary_cen+env.boundary_ext;
	RXCOUT << "simlation range : " << m_v3EnvMin << " - " << m_v3EnvMax << endl;

	m_params.Boundary[0] = MAKE_FLOAT3V(m_v3EnvMin);
	m_params.Boundary[1] = MAKE_FLOAT3V(m_v3EnvMax);

	// �ő̃p�[�e�B�N�������p
	rxSolidBox *box = new rxSolidBox(env.boundary_cen-env.boundary_ext, env.boundary_cen+env.boundary_ext, -1);
	m_vSolids.push_back(box);
	//m_vSolids.back()->IsSolidParticles() = (env.boundary_particle ? true : false);

	// �V�~�����[�V�������E�p�[�e�B�N���̐���
	m_vSolids.back()->IsSolidParticles() = true;	// ����OFF�̏ꍇ�͂�����false�ɂ���


	Vec3 world_size = m_v3EnvMax-m_v3EnvMin;
	Vec3 world_origin = m_v3EnvMin;

	double expansion = 0.01;
	world_origin -= 0.5*expansion*world_size;
	world_size *= (1.0+expansion); // �V�~�����[�V�������S�̂𕢂��悤�ɐݒ�

	m_v3EnvMin = world_origin;
	m_v3EnvMax = world_origin+world_size;

	// �����Z���ݒ�
	double cell_width;
	setupCells(m_dCellData, m_gridSize, cell_width, m_v3EnvMin, m_v3EnvMax, h);
	m_params.CellWidth = MAKE_FLOAT3(cell_width, cell_width, cell_width);

	m_gridSortBits = 24;	// �O���b�h�����������͂��̒l�𑝂₷

	m_params.GridSize = m_gridSize;
	m_params.NumCells = m_dCellData.uNumCells;
	m_params.NumBodies = m_uNumParticles;

	m_params.WorldOrigin = MAKE_FLOAT3(world_origin[0], world_origin[1], world_origin[2]);
	m_params.WorldMax = MAKE_FLOAT3(m_v3EnvMax[0], m_v3EnvMax[1], m_v3EnvMax[2] );

	RXCOUT << "grid for nn search : " << endl;
	RXCOUT << "  size   : " << m_params.GridSize << endl;
	RXCOUT << "  num    : " << m_params.NumCells << endl;
	RXCOUT << "  origin : " << m_params.WorldOrigin << endl;
	RXCOUT << "  width  : " << m_params.CellWidth << endl;

	m_uNumArdGrid = (int)(m_params.EffectiveRadius/m_params.CellWidth.x)+1;
	RXCOUT << "  numArdGrid : " << m_uNumArdGrid << endl;

	// GPU�ɓn�����߂̕����Z�����
	m_dCellData.uNumArdGrid = m_uNumArdGrid;
	m_dCellData.uNumCells = m_dCellData.uNumCells;
	m_dCellData.uNumParticles = m_uNumParticles;

	m_uNumParticles = 0;

	Allocate(env.max_particles);
}

inline uint calUintPow(uint x, uint y)
{
	uint x_y = 1;
	for(uint i=0; i < y;i++) x_y *= x;
	return x_y;
}

/*!
 * �������̊m��
 *  - �ő�p�[�e�B�N�����Ŋm��
 * @param[in] max_particles �ő�p�[�e�B�N����
 */
void rxPBDSPH_GPU::Allocate(int maxParticles)
{
	// MARK:Allocate
	assert(!m_bInitialized);

	//m_uNumParticles = maxParticles;
	m_uMaxParticles = maxParticles;

	unsigned int size  = m_uMaxParticles*DIM;
	unsigned int size1 = m_uMaxParticles;
	unsigned int mem_size  = sizeof(RXREAL)*size;
	unsigned int mem_size1 = sizeof(RXREAL)*size1;


	//
	// CPU���������m��
	//
	// GPU�Ƃ̃f�[�^�]���������f�[�^�̓y�[�W���b�N�������Ɋm��
	cudaMallocHost((void**)&m_hPos, mem_size);
	cudaMallocHost((void**)&m_hVel, mem_size);

	// �ʏ�̃������m��
	m_hFrc = new RXREAL[size];
	m_hDens = new RXREAL[size1];
	memset(m_hFrc, 0, mem_size);
	memset(m_hDens, 0, mem_size1);

	m_hS = new RXREAL[size1];
	m_hDp = new RXREAL[size];
	m_hPredictPos = new RXREAL[size];
	m_hPredictVel = new RXREAL[size];
	memset(m_hS, 0, mem_size1);
	memset(m_hDp, 0, mem_size);
	memset(m_hPredictPos, 0, mem_size);
	memset(m_hPredictVel, 0, mem_size);

	m_hTmp = new RXREAL[m_uMaxParticles];
	memset(m_hTmp, 0, sizeof(RXREAL)*m_uMaxParticles);


	//
	// GPU���������m��
	//
	if(m_bUseOpenGL){
		m_posVBO = createVBO(mem_size);	
		CuRegisterGLBufferObject(m_posVBO, &m_pPosResource);

		m_colorVBO = createVBO(sizeof(RXREAL)*4*m_uMaxParticles);
		SetColorVBO(RX_RAMP, -1);
	}
	else{
		CuAllocateArray((void**)&m_dPos, mem_size);
	}

	CuAllocateArray((void**)&m_dVel,    mem_size);
	CuAllocateArray((void**)&m_dFrc,    mem_size);
	CuAllocateArray((void**)&m_dDens,   mem_size1);

	CuAllocateArray((void**)&m_dS,  mem_size1);
	CuAllocateArray((void**)&m_dDp, mem_size);
	CuAllocateArray((void**)&m_dPredictPos, mem_size);
	CuAllocateArray((void**)&m_dPredictVel, mem_size);

	CuAllocateArray((void**)&m_dErr, mem_size1);
	CuAllocateArray((void**)&m_dErrScan, mem_size1);

	// �p�[�e�B�N���O���b�h�f�[�^
	CuAllocateArray((void**)&m_dCellData.dSortedPos, mem_size);
	CuAllocateArray((void**)&m_dCellData.dSortedVel, mem_size);
	CuAllocateArray((void**)&m_dCellData.dGridParticleHash,  m_uMaxParticles*sizeof(uint));
	CuAllocateArray((void**)&m_dCellData.dSortedIndex, m_uMaxParticles*sizeof(uint));
	CuAllocateArray((void**)&m_dCellData.dCellStart, m_dCellData.uNumCells*sizeof(uint));
	CuAllocateArray((void**)&m_dCellData.dCellEnd, m_dCellData.uNumCells*sizeof(uint));

	CuSetParameters(&m_params);

	m_bInitialized = true;
}

/*!
 * �m�ۂ����������̉��
 */
void rxPBDSPH_GPU::Finalize(void)
{
	assert(m_bInitialized);

	// �y�[�W���b�N���������
	cudaFreeHost(m_hPos);
	cudaFreeHost(m_hVel);

	// �ʏ탁�������
	if(m_hFrc) delete [] m_hFrc;
	if(m_hDens) delete [] m_hDens;
	if(m_hS) delete [] m_hS;
	if(m_hDp) delete [] m_hDp;

	if(m_hPosB) delete [] m_hPosB;
	if(m_hVolB) delete [] m_hVolB;
	if(m_hSb) delete [] m_hSb;

	if(m_hPredictPos) delete [] m_hPredictPos;
	if(m_hPredictVel) delete [] m_hPredictVel;

	if(m_hTmp) delete [] m_hTmp;

	// �ő̃p�[�e�B�N�������p�ő�
	int num_solid = (int)m_vSolids.size();
	for(int i = 0; i < num_solid; ++i){
		delete m_vSolids[i];
	}
	m_vSolids.clear();	


	// GPU���������
	CuFreeArray(m_dVel);
	CuFreeArray(m_dFrc);
	CuFreeArray(m_dDens);

	CuFreeArray(m_dPosB);
	CuFreeArray(m_dVolB);
	CuFreeArray(m_dSb);

	CuFreeArray(m_dS);
	CuFreeArray(m_dDp);
	CuFreeArray(m_dPredictPos);
	CuFreeArray(m_dPredictVel);

	CuFreeArray(m_dErr);
	CuFreeArray(m_dErrScan);

	CuFreeArray(m_dCellData.dSortedPos);
	CuFreeArray(m_dCellData.dSortedVel);
	CuFreeArray(m_dCellData.dGridParticleHash);
	CuFreeArray(m_dCellData.dSortedIndex);
	CuFreeArray(m_dCellData.dCellStart);
	CuFreeArray(m_dCellData.dCellEnd);

	CuFreeArray(m_dCellDataB.dSortedPos);
	CuFreeArray(m_dCellDataB.dSortedVel);
	CuFreeArray(m_dCellDataB.dGridParticleHash);
	CuFreeArray(m_dCellDataB.dSortedIndex);
	CuFreeArray(m_dCellDataB.dCellStart);
	CuFreeArray(m_dCellDataB.dCellEnd);

	if(m_dVrts) CuFreeArray(m_dVrts);
	if(m_dTris) CuFreeArray(m_dTris);

	if(m_dCellData.dSortedPolyIdx) CuFreeArray(m_dCellData.dSortedPolyIdx);
	if(m_dCellData.dGridPolyHash)  CuFreeArray(m_dCellData.dGridPolyHash);
	if(m_dCellData.dPolyCellStart) CuFreeArray(m_dCellData.dPolyCellStart);
	if(m_dCellData.dPolyCellEnd)   CuFreeArray(m_dCellData.dPolyCellEnd);

	if(m_dCellDataB.dSortedPolyIdx) CuFreeArray(m_dCellDataB.dSortedPolyIdx);
	if(m_dCellDataB.dGridPolyHash)  CuFreeArray(m_dCellDataB.dGridPolyHash);
	if(m_dCellDataB.dPolyCellStart) CuFreeArray(m_dCellDataB.dPolyCellStart);
	if(m_dCellDataB.dPolyCellEnd)   CuFreeArray(m_dCellDataB.dPolyCellEnd);

	if(m_bUseOpenGL){
		CuUnregisterGLBufferObject(m_pPosResource);
		glDeleteBuffers(1, (const GLuint*)&m_posVBO);
		glDeleteBuffers(1, (const GLuint*)&m_colorVBO);
	}
	else{
		CuFreeArray(m_dPos);
	}

	m_uNumParticles = 0;
	m_uMaxParticles = 0;
}


/*!
 * ���E�p�[�e�B�N���̐����Ƃ��̉��z�̐ς̌v�Z
 */
void rxPBDSPH_GPU::InitBoundary(void)
{
	// �������ꂽ���E�p�[�e�B�N���̐����i�[����z����m��
	vector<int> n(m_vSolids.size());
	for(size_t i = 0; i < m_vSolids.size(); ++i){
		n[i] = 0;
	}

	// ���E�p�[�e�B�N������
	RXREAL scale = 0.85;
	if(!m_vSolids.empty()){
		vector<RXREAL*> posb(m_vSolids.size());

		m_uNumBParticles = 0;
		for(int i = 0; i < m_vSolids.size(); ++i){
			if(!(m_vSolids[i]->IsSolidParticles())) continue;
			n[i] = m_vSolids[i]->GenerateParticlesOnSurf(scale*m_fParticleRadius, &posb[i]);
			m_uNumBParticles += n[i];
		}

		m_hPosB = new RXREAL[m_uNumBParticles*DIM];
		int accN = 0, s = 0;
		for(int i = 0; i < (int)m_uNumBParticles; ++i){
			if(i >= accN+n[s]){
				accN += n[s];
				s++;
			}
			for(int j = 0; j < DIM; ++j){
				m_hPosB[DIM*i+j] = posb[s][DIM*(i-accN)+j];
			}
		}
		if(!posb.empty()){
			for(size_t i = 0; i < m_vSolids.size(); ++i){
				if(!(m_vSolids[i]->IsSolidParticles())) continue;
				if(posb[i]) delete [] posb[i];
			}
			posb.clear();
		}
	}
	m_uNumBParticles0 = n[0];


	if(m_uNumBParticles){
		unsigned int sizeb  = m_uNumBParticles*DIM;
		unsigned int mem_sizeb  = sizeof(RXREAL)*sizeb;

		Vec3 minp = m_v3EnvMin-Vec3(4.0*m_fParticleRadius);
		Vec3 maxp = m_v3EnvMax+Vec3(4.0*m_fParticleRadius);

		// �����Z���ݒ�
		double cell_width;
		setupCells(m_dCellDataB, m_gridSizeB, cell_width, minp, maxp, m_params.EffectiveRadius);
		m_params.CellWidthB = MAKE_FLOAT3(cell_width, cell_width, cell_width);

		m_params.GridSizeB = m_gridSizeB;
		m_params.NumCellsB = m_dCellDataB.uNumCells;
		m_params.NumBodiesB = m_uNumBParticles;

		m_params.WorldOriginB = MAKE_FLOAT3(minp[0], minp[1], minp[2]);
		m_params.WorldMaxB = MAKE_FLOAT3(maxp[0], maxp[1], maxp[2] );

		RXCOUT << "grid for nn search (boundary) : " << endl;
		RXCOUT << "  size   : " << m_params.GridSizeB << endl;
		RXCOUT << "  num    : " << m_params.NumCellsB << endl;
		RXCOUT << "  origin : " << m_params.WorldOriginB << endl;
		RXCOUT << "  width  : " << m_params.CellWidthB << endl;

		// GPU�ɓn�����߂̕����Z�����
		m_dCellDataB.uNumArdGrid = m_uNumArdGrid;
		m_dCellDataB.uNumParticles = m_uNumBParticles;

		// ���E�p�[�e�B�N���O���b�h�f�[�^
		CuAllocateArray((void**)&m_dCellDataB.dSortedPos, mem_sizeb);
		CuAllocateArray((void**)&m_dCellDataB.dSortedVel, mem_sizeb);
		CuAllocateArray((void**)&m_dCellDataB.dGridParticleHash,  m_uNumBParticles*sizeof(uint));
		CuAllocateArray((void**)&m_dCellDataB.dSortedIndex, m_uNumBParticles*sizeof(uint));
		CuAllocateArray((void**)&m_dCellDataB.dCellStart, m_dCellDataB.uNumCells*sizeof(uint));
		CuAllocateArray((void**)&m_dCellDataB.dCellEnd, m_dCellDataB.uNumCells*sizeof(uint));

		// ���E�p�[�e�B�N���̈ʒu�����f�o�C�X�ɓ]��
		CuAllocateArray((void**)&m_dPosB, mem_sizeb);
		CuCopyArrayToDevice(m_dPosB, m_hPosB, 0, m_uNumBParticles*DIM*sizeof(RXREAL));

		
		CuSetParameters(&m_params);

		// �ߖT�T���������p�O���b�h�f�[�^�̍쐬
		// �����Z���̃n�b�V�����v�Z
		CuCalcHashB(m_dCellDataB.dGridParticleHash, m_dCellDataB.dSortedIndex, m_dPosB, 
					m_params.WorldOriginB, m_params.CellWidthB, m_params.GridSizeB, m_uNumBParticles);


		// �n�b�V���Ɋ�Â��p�[�e�B�N�����\�[�g
		CuSort(m_dCellDataB.dGridParticleHash, m_dCellDataB.dSortedIndex, m_uNumBParticles);

		// �p�[�e�B�N���z����\�[�g���ꂽ���Ԃɕ��ёւ��C
		// �e�Z���̎n�܂�ƏI���̃C���f�b�N�X������
		CuReorderDataAndFindCellStartB(m_dCellDataB, m_dPosB);

		// ���E�p�[�e�B�N���̑̐�
		m_hVolB = new RXREAL[m_uNumBParticles];
		memset(m_hVolB, 0, sizeof(RXREAL)*m_uNumBParticles);
		CuAllocateArray((void**)&m_dVolB, sizeof(RXREAL)*m_uNumBParticles);

		CuSphBoundaryVolume(m_dVolB, m_params.Mass, m_dCellDataB);

		CuCopyArrayFromDevice(m_hVolB, m_dVolB, 0, 0, m_uNumBParticles*sizeof(RXREAL));

		// ���E�p�[�e�B�N���̃X�P�[�����O�t�@�N�^
		m_hSb = new RXREAL[m_uNumBParticles];
		memset(m_hSb, 0, sizeof(RXREAL)*m_uNumBParticles);
		CuAllocateArray((void**)&m_dSb, sizeof(RXREAL)*m_uNumBParticles);
	}
}



/*!
 * GPU���̃p�����[�^�t�@�C�����X�V
 */
void rxPBDSPH_GPU::UpdateParams(void)
{
	m_params.ParticleRadius = m_fParticleRadius;
	m_params.Gravity = make_float3(m_v3Gravity[0], m_v3Gravity[1], m_v3Gravity[2]);

	m_params.AP = (m_bArtificialPressure ? 1 : 0);

	m_params.Restitution = m_fRestitution;
	CuSetParameters(&m_params);
}

/*!
 * SPH��1�X�e�b�v�i�߂�
 * @param[in] dt ���ԃX�e�b�v��
 * @retval ture  �v�Z����
 * @retval false �ő�X�e�b�v���𒴂��Ă��܂�
 */
bool rxPBDSPH_GPU::Update(RXREAL dt, int step)
{
	//RXTIMER_RESET;

	// �����p�[�e�B�N����ǉ�
	if(!m_vInletLines.empty()){
		int start = (m_iInletStart == -1 ? 0 : m_iInletStart);
		int num = 0;
		vector<rxInletLine>::iterator itr = m_vInletLines.begin();
		for(; itr != m_vInletLines.end(); ++itr){
			rxInletLine iline = *itr;
			if(iline.span > 0 && step%(iline.span) == 0){
				int count = addParticles(m_iInletStart, iline);
				num += count;
			}
		}
		SetArrayVBO(RX_POSITION, m_hPos, start, num);
		SetArrayVBO(RX_VELOCITY, m_hVel, start, num);
	}

	if(!m_uNumParticles) return false;

	assert(m_bInitialized);

	static bool init = true;

	m_params.Dt = dt;
	UpdateParams();

	// GPU�p�ϐ��Ƀp�[�e�B�N������ݒ�
	m_params.NumBodies = m_uNumParticles;
	m_dCellData.uNumParticles = m_uNumParticles;

	// �p�[�e�B�N�����W�z����}�b�s���O
	RXREAL *dPos;
	if(m_bUseOpenGL){
		dPos = (RXREAL*)CuMapGLBufferObject(&m_pPosResource);
	}
	else{
		dPos = (RXREAL*)m_dPos;
	}


	// �ߖT���q�T��
	setObjectToCell(dPos, m_dVel);

	// ���x�v�Z
	CuPbfDensity(m_dDens, m_dCellData);

	// ���E�p�[�e�B�N���ɂ�閧�x
	CuPbfBoundaryDensity(m_dDens, dPos, m_dVolB, m_dCellDataB, m_uNumParticles);

	//if(!init){	// �^�C���X�e�b�v���̏C��
	//	dt = calTimeStep(dt, eta, m_hFrc, m_hVel, m_hDens);
	//}

	// �O�͍��ɂ��͏�̌v�Z
	CuPbfExternalForces(m_dDens, m_dFrc, m_dCellData, dt);

	// �\���ʒu�C���x�̌v�Z
	if(m_iNumTris == 0){
		CuPbfIntegrate(dPos, m_dVel, m_dFrc, m_dPredictPos, m_dPredictVel, dt, m_uNumParticles);
	}
	else{	// �|���S���ɂ��ő̋��E�L��
		CuPbfIntegrateWithPolygon(dPos, m_dVel, m_dFrc, m_dPredictPos, m_dPredictVel, 
									 m_dVrts, m_dTris, m_iNumTris, dt, m_dCellData);
	}
	RXTIMER("force calculation");

	// ����
	int iter = 0;	// ������
	RXREAL dens_var = 1.0;	// ���x�̕��U
	while(((dens_var > m_fEta) || (iter < m_iMinIterations)) && (iter < m_iMaxIterations)){
		// �ߖT���q�T��
		setObjectToCell(m_dPredictPos, m_dPredictVel);

		// �S������C�ƃX�P�[�����O�t�@�N�^s�̌v�Z
		CuPbfScalingFactorWithBoundary(m_dPredictPos, m_dDens, m_dS, m_fEpsilon, m_dCellData, m_dVolB, m_dSb, m_dCellDataB);

		// �ʒu�C���ʃ�p�̌v�Z
		CuPbfPositionCorrectionWithBoundary(m_dPredictPos, m_dS, m_dDp, m_dCellData, m_dVolB, m_dSb, m_dCellDataB);
		
		// �p�[�e�B�N���ʒu�C��
		CuPbfCorrectPosition(m_dPredictPos, m_dDp, m_uNumParticles);

		// integrate�ŏՓˏ��������ɂ��邽�߂�0�ŏ�����
		CuSetArrayValue(m_dPredictVel, 0, sizeof(RXREAL)*m_uMaxParticles*DIM);
		CuSetArrayValue(m_dFrc, 0, sizeof(RXREAL)*m_uMaxParticles*DIM);

		// �Փˏ���
		if(m_iNumTris == 0){
			CuPbfIntegrate2(dPos, m_dVel, m_dFrc, m_dPredictPos, m_dPredictVel, dt, m_uNumParticles);
		}
		else{	// �|���S���ɂ��ő̋��E�L��
			CuPbfIntegrateWithPolygon2(dPos, m_dVel, m_dFrc, m_dPredictPos, m_dPredictVel, 
										  m_dVrts, m_dTris, m_iNumTris, dt, m_dCellData);
		}

		// ���ϖ��x�ϓ��̌v�Z
		dens_var = CuPbfCalDensityFluctuation(m_dErrScan, m_dErr, m_dDens, m_params.Density, m_uNumParticles);

		if(dens_var <= m_fEta && iter > m_iMinIterations) break;

		iter++;
	}

	g_iIterations = iter;
	g_fEta = dens_var;


	// ���x�E�ʒu�X�V
	CuPbfUpdateVelocity(m_dPredictPos, dPos, m_dPredictVel, dt, m_uNumParticles);
	//setObjectToCell(dPos, m_dPredictVel);
	//CuXSphViscosity(dPos, m_dPredictVel, m_dVel, m_dDens, 0.001, m_dCellData);
	CuCopyArrayD2D(m_dVel, m_dPredictVel, sizeof(RXREAL)*m_uMaxParticles*DIM);
	CuCopyArrayD2D(dPos, m_dPredictPos, sizeof(RXREAL)*m_uMaxParticles*DIM);

	RXTIMER("update position");



	init = false;

	if(m_bUseOpenGL){
		CuUnmapGLBufferObject(m_pPosResource);
	}

	m_fTime += dt;

	// VBO���烁������
	//m_hPos = GetArrayVBO(RX_POSITION);
	//m_hVel = GetArrayVBO(RX_VELOCITY);

	SetColorVBO(m_iColorType, -1);

	return true;
}


/*!
 * rest density�̌v�Z
 *  - �ߖT�Ƀp�[�e�B�N�����~���l�߂��Ă���Ƃ��Ė��x���v�Z����
 * @param[in] h �L�����a
 * @return rest density
 */
RXREAL rxPBDSPH_GPU::calRestDensity(RXREAL h)
{
	double a = KernelCoefPoly6(h, 3, 1);
	RXREAL r0 = 0.0;
	RXREAL l = 2*GetParticleRadius();
	int n = (int)ceil(m_params.EffectiveRadius/l)+1;
	for(int x = -n; x <= n; ++x){
		for(int y = -n; y <= n; ++y){
			for(int z = -n; z <= n; ++z){
				Vec3 rij = Vec3(x*l, y*l, z*l);
				r0 += m_params.Mass*KernelPoly6(norm(rij), h, a);
			}
		}
	}
	return r0;
}


/*!
 * �p�[�e�B�N���f�[�^�̎擾
 * @return �p�[�e�B�N���f�[�^�̃f�o�C�X�������|�C���^
 */
RXREAL* rxPBDSPH_GPU::GetParticle(void)
{
	return m_hPos;
}

/*!
 * �p�[�e�B�N���f�[�^�̎擾
 * @return �p�[�e�B�N���f�[�^�̃f�o�C�X�������|�C���^
 */
RXREAL* rxPBDSPH_GPU::GetParticleDevice(void)
{
	return (m_bUseOpenGL ? (RXREAL*)CuMapGLBufferObject(&m_pPosResource) : (RXREAL*)m_dPos);
}
/*!
 * �p�[�e�B�N���f�[�^�̃A���}�b�v(VBO)
 */
void rxPBDSPH_GPU::UnmapParticle(void)
{
	if(m_bUseOpenGL) CuUnmapGLBufferObject(m_pPosResource);
}




/*!
 * �J���[�l�pVBO�̕ҏW
 * @param[in] type �F�̃x�[�X�Ƃ��镨���l
 */
void rxPBDSPH_GPU::SetColorVBO(int type, int picked)
{
	// MRK:SetColorVBO

	switch(type){
	case RX_DENSITY:
		CuCopyArrayFromDevice(m_hDens, m_dDens, 0, 0, sizeof(RXREAL)*m_uNumParticles);
		SetColorVBOFromArray(m_hDens, 1, false, m_params.Density*3);
		break;

	case RX_CONSTANT:
		if(m_bUseOpenGL){
			// �J���[�o�b�t�@�ɒl��ݒ�
			glBindBufferARB(GL_ARRAY_BUFFER, m_colorVBO);
			RXREAL *data = (RXREAL*)glMapBufferARB(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
			RXREAL *ptr = data;
			for(uint i = 0; i < m_uNumParticles; ++i){
				*ptr++ = 0.25f;
				*ptr++ = 0.25f;
				*ptr++ = 0.95f;
				*ptr++ = 1.0f;
			}
			glUnmapBufferARB(GL_ARRAY_BUFFER);
		}
		break;

	case RX_RAMP:
		if(m_bUseOpenGL){
			// �J���[�o�b�t�@�ɒl��ݒ�
			glBindBufferARB(GL_ARRAY_BUFFER, m_colorVBO);
			RXREAL *data = (RXREAL*)glMapBufferARB(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
			RXREAL *ptr = data;
			for(uint i = 0; i < m_uNumParticles; ++i){
				RXREAL t = i/(RXREAL)m_uNumParticles;
#if 0
				*ptr++ = rand()/(RXREAL)RAND_MAX;
				*ptr++ = rand()/(RXREAL)RAND_MAX;
				*ptr++ = rand()/(RXREAL)RAND_MAX;
#else
				RX_COLOR_RAMP(t, ptr);
				ptr += 3;
#endif
				*ptr++ = 1.0f;
			}
			glUnmapBufferARB(GL_ARRAY_BUFFER);
		}
		break;

	default:
		break;
	}

}


void rxPBDSPH_GPU::CalMaxDensity(int k)
{
	CuCopyArrayFromDevice(m_hDens, m_dDens, 0, 0, sizeof(RXREAL)*2*m_uNumParticles);

	RXREAL max_dens = 0.0;
	for(uint i = 0; i < m_uNumParticles; ++i){
		if(m_hDens[2*i+k] > max_dens) max_dens = m_hDens[2*i+k];
	}

	RXCOUT << "Density  : " << max_dens << endl;
}


/*!
 * �O���b�h�n�b�V���l�̌v�Z
 * @param[in] x,y,z �O���b�h�ʒu
 * @return �O���b�h�n�b�V���l
 */
uint rxPBDSPH_GPU::calGridHash(uint x, uint y, uint z)
{
	x = (x < 0 ? 0 : (x >= m_params.GridSize.x ? m_params.GridSize.x-1 : x));
	y = (y < 0 ? 0 : (y >= m_params.GridSize.y ? m_params.GridSize.y-1 : y));
	z = (z < 0 ? 0 : (z >= m_params.GridSize.z ? m_params.GridSize.z-1 : z));
	return z*m_params.GridSize.y*m_params.GridSize.x+y*m_params.GridSize.x+x;
}
/*!
 * �O���b�h�n�b�V���l�̌v�Z
 * @param[in] pos �p�[�e�B�N�����W
 * @return �O���b�h�n�b�V���l
 */
uint rxPBDSPH_GPU::calGridHash(Vec3 pos)
{
	pos -= m_v3EnvMin;

	// �����Z���C���f�b�N�X�̎Z�o
	uint x = pos[0]/m_params.CellWidth.x;
	uint y = pos[1]/m_params.CellWidth.y;
	uint z = pos[2]/m_params.CellWidth.z;
	return calGridHash(x, y, z);
}

/*!
 * ��ԕ����@�̏���
 * @param[out] cell �����O���b�h�f�[�^
 * @param[out] gridsize �e���̃O���b�h��
 * @param[out] cell_width �Z����
 * @param[in] vMin ���̍ŏ����W
 * @param[in] vMax ���̍ő���W
 * @param[in] h �e�����a
 */
void rxPBDSPH_GPU::setupCells(rxParticleCell &cell, uint3 &gridsize, double &cell_width, Vec3 vMin, Vec3 vMax, double h)
{
	if(h < RX_EPS) return;

	Vec3 world_size = vMax-vMin;
	Vec3 world_origin = vMin;

	double max_axis = RXFunc::Max3(world_size);

	int d = (int)(log(max_axis/h)/log(2.0)+0.5);
	int n = (int)(pow(2.0, (double)d)+0.5);
	cell_width = max_axis/n;

	gridsize.x = (int)(world_size[0]/cell_width)+1;
	gridsize.y = (int)(world_size[1]/cell_width)+1;
	gridsize.z = (int)(world_size[2]/cell_width)+1;

	cell.uNumCells = gridsize.x*gridsize.y*gridsize.z;
}


/*!
 * �S�p�[�e�B�N���𕪊��Z���Ɋi�[
 *  - �e�p�[�e�B�N���̑�����O���b�h�n�b�V�����v�Z���Ċi�[����
 * @param[in] p �S�p�[�e�B�N���̍��W���i�[�����z��
 */
void rxPBDSPH_GPU::setObjectToCell(RXREAL *p, RXREAL *v)
{
	// �ߖT�T���������p�O���b�h�f�[�^�̍쐬
	// �����Z���̃n�b�V�����v�Z
	CuCalcHash(m_dCellData.dGridParticleHash, m_dCellData.dSortedIndex, p, m_uNumParticles);

	// �n�b�V���Ɋ�Â��p�[�e�B�N�����\�[�g
	CuSort(m_dCellData.dGridParticleHash, m_dCellData.dSortedIndex, m_uNumParticles);

	// �p�[�e�B�N���z����\�[�g���ꂽ���Ԃɕ��ёւ��C
	// �e�Z���̎n�܂�ƏI���̃C���f�b�N�X������
	CuReorderDataAndFindCellStart(m_dCellData, p, v);
}

/*!
 * �S�p�[�e�B�N���𕪊��Z���Ɋi�[
 */
void rxPBDSPH_GPU::SetParticlesToCell(RXREAL *prts, int n, RXREAL h)
{
}
void rxPBDSPH_GPU::SetParticlesToCell(void)
{
	SetParticlesToCell(m_hPos, m_uNumParticles, m_params.EffectiveRadius);
}

/*!
 * �|���S���𕪊��Z���Ɋi�[
 * @param[in] vrts �|���S�����_
 * @param[in] nv ���_��
 * @param[in] tris ���b�V��
 * @param[in] nt ���b�V����
 */
void rxPBDSPH_GPU::setPolysToCell(RXREAL *vrts, int nv, int* tris, int nt)
{
	// MRK:setPolysToCell
	uint *hPolyCellStart = new uint[m_dCellData.uNumCells];
	uint *hPolyCellEnd = new uint[m_dCellData.uNumCells];

	int mem_size2 = m_dCellData.uNumCells*sizeof(uint);
	memset(hPolyCellStart, 0xffffffff, mem_size2);
	memset(hPolyCellEnd,   0,          mem_size2);

	int num_hash = 0;

	// �e�p�[�e�B�N���̃O���b�h�n�b�V���̌v�Z
	vector<uint> tri_hash, tri_idx;
	vector<Vec3> tri_vrts, tri_vrts_c;
	tri_vrts.resize(3);
	tri_vrts_c.resize(3);
	for(int i = 0; i < nt; i++){
		for(int j = 0; j < 3; ++j){
			Vec3 pos;
			pos[0] = vrts[3*tris[3*i+j]+0];
			pos[1] = vrts[3*tris[3*i+j]+1];
			pos[2] = vrts[3*tris[3*i+j]+2];
			tri_vrts[j] = pos;
		}

		Vec3 nrm = Unit(cross(tri_vrts[1]-tri_vrts[0], tri_vrts[2]-tri_vrts[0]));

		// �|���S����BBox
		Vec3 bmin, bmax;
		bmin = tri_vrts[0];
		bmax = tri_vrts[0];
		for(int j = 1; j < 3; ++j){
			for(int k = 0; k < 3; ++k){
				if(tri_vrts[j][k] < bmin[k]) bmin[k] = tri_vrts[j][k];
				if(tri_vrts[j][k] > bmax[k]) bmax[k] = tri_vrts[j][k];
			}
		}

		// BBox�Əd�Ȃ�Z��
		bmin -= m_v3EnvMin;
		bmax -= m_v3EnvMin;

		// �����Z���C���f�b�N�X�̎Z�o
		int bmin_gidx[3], bmax_gidx[3];
		bmin_gidx[0] = bmin[0]/m_params.CellWidth.x;
		bmin_gidx[1] = bmin[1]/m_params.CellWidth.y;
		bmin_gidx[2] = bmin[2]/m_params.CellWidth.z;
		bmax_gidx[0] = bmax[0]/m_params.CellWidth.x;
		bmax_gidx[1] = bmax[1]/m_params.CellWidth.y;
		bmax_gidx[2] = bmax[2]/m_params.CellWidth.z;

		bmin_gidx[0] = RX_CLAMP(bmin_gidx[0], 0, (int)m_params.GridSize.x-1);
		bmin_gidx[1] = RX_CLAMP(bmin_gidx[1], 0, (int)m_params.GridSize.y-1);
		bmin_gidx[2] = RX_CLAMP(bmin_gidx[2], 0, (int)m_params.GridSize.z-1);
		bmax_gidx[0] = RX_CLAMP(bmax_gidx[0], 0, (int)m_params.GridSize.x-1);
		bmax_gidx[1] = RX_CLAMP(bmax_gidx[1], 0, (int)m_params.GridSize.y-1);
		bmax_gidx[2] = RX_CLAMP(bmax_gidx[2], 0, (int)m_params.GridSize.z-1);

		// �e�Z���Ƀ|���S�����܂܂�邩���`�F�b�N
		Vec3 len = Vec3(m_params.CellWidth.x, m_params.CellWidth.y, m_params.CellWidth.z);
		Vec3 cen(0.0);
		for(int x = bmin_gidx[0]; x <= bmax_gidx[0]; ++x){
			for(int y = bmin_gidx[1]; y <= bmax_gidx[1]; ++y){
				for(int z = bmin_gidx[2]; z <= bmax_gidx[2]; ++z){
					cen = m_v3EnvMin+Vec3(x+0.5, y+0.5, z+0.5)*len;

					for(int j = 0; j < 3; ++j){
						tri_vrts_c[j] = (tri_vrts[j]-cen)/len;
					}

					if(RXFunc::polygon_intersects_cube(tri_vrts_c, nrm)){
						// �n�b�V���l�v�Z
						uint hash = calGridHash(x, y, z);

						tri_idx.push_back((uint)i);
						tri_hash.push_back(hash);

						num_hash++;
					}
				}
			}
		}
	}

	RXCOUT << "polygon hash : " << num_hash << endl;

	m_dCellData.uNumPolyHash = (uint)num_hash;

	if(num_hash){
		int mem_size1 = m_dCellData.uNumPolyHash*sizeof(uint);
		uint *hSortedPolyIdx = new uint[m_dCellData.uNumPolyHash];
		uint *hGridPolyHash  = new uint[m_dCellData.uNumPolyHash];
		memcpy(hSortedPolyIdx, &tri_idx[0], mem_size1);
		memcpy(hGridPolyHash, &tri_hash[0], mem_size1);

		// �O���b�h�n�b�V���Ń\�[�g
		if(m_dCellData.dSortedPolyIdx) CuFreeArray(m_dCellData.dSortedPolyIdx);
		if(m_dCellData.dGridPolyHash) CuFreeArray(m_dCellData.dGridPolyHash);
		CuAllocateArray((void**)&m_dCellData.dSortedPolyIdx, mem_size1);
		CuAllocateArray((void**)&m_dCellData.dGridPolyHash,  mem_size1);

		CuCopyArrayToDevice(m_dCellData.dSortedPolyIdx, &tri_idx[0],  0, mem_size1);
		CuCopyArrayToDevice(m_dCellData.dGridPolyHash,  &tri_hash[0], 0, mem_size1);

		CuSort(m_dCellData.dGridPolyHash, m_dCellData.dSortedPolyIdx, m_dCellData.uNumPolyHash);

		CuCopyArrayFromDevice(hSortedPolyIdx, m_dCellData.dSortedPolyIdx, 0, 0, mem_size1);
		CuCopyArrayFromDevice(hGridPolyHash,  m_dCellData.dGridPolyHash,  0, 0, mem_size1);

		// �p�[�e�B�N���z����\�[�g���ꂽ���Ԃɕ��ёւ��C
		// �e�Z���̎n�܂�ƏI���̃C���f�b�N�X������
		for(uint i = 0; i < m_dCellData.uNumPolyHash; ++i){
			uint hash = hGridPolyHash[i];

			if(i == 0){
				hPolyCellStart[hash] = i;
			}
			else{
				uint prev_hash = hGridPolyHash[i-1];

				if(i == 0 || hash != prev_hash){
					hPolyCellStart[hash] = i;
					if(i > 0){
						hPolyCellEnd[prev_hash] = i;
					}
				}

				if(i == m_uNumParticles-1){
					hPolyCellEnd[hash] = i+1;
				}
			}
		}

		if(m_dCellData.dPolyCellStart) CuFreeArray(m_dCellData.dPolyCellStart);
		if(m_dCellData.dPolyCellEnd) CuFreeArray(m_dCellData.dPolyCellEnd);

		CuAllocateArray((void**)&m_dCellData.dPolyCellStart, mem_size2);
		CuAllocateArray((void**)&m_dCellData.dPolyCellEnd,   mem_size2);

		CuCopyArrayToDevice(m_dCellData.dPolyCellStart,  hPolyCellStart, 0, mem_size2);
		CuCopyArrayToDevice(m_dCellData.dPolyCellEnd,    hPolyCellEnd,   0, mem_size2);

		delete [] hGridPolyHash;
		delete [] hSortedPolyIdx;
	}

	delete [] hPolyCellStart;
	delete [] hPolyCellEnd;
}

/*!
 * �|���S���𕪊��Z���Ɋi�[
 */
void rxPBDSPH_GPU::SetPolygonsToCell(void)
{
	setPolysToCell(&m_vVrts[0], m_iNumVrts, &m_vTris[0], m_iNumTris);
}


/*!
 * �T���p�Z���̕`��
 * @param[in] i,j,k �O���b�h��̃C���f�b�N�X
 */
void rxPBDSPH_GPU::DrawCell(int i, int j, int k)
{
	glPushMatrix();
	glTranslated(m_v3EnvMin[0], m_v3EnvMin[1], m_v3EnvMin[2]);
	glTranslatef((i+0.5)*m_params.CellWidth.x, (j+0.5)*m_params.CellWidth.y, (k+0.5)*m_params.CellWidth.z);
	glutWireCube(m_params.CellWidth.x);
	glPopMatrix();
}


/*!
 * �T���p�O���b�h�̕`��
 * @param[in] col �p�[�e�B�N�����܂܂��Z���̐F
 * @param[in] col2 �|���S�����܂܂��Z���̐F
 * @param[in] sel �����_���ɑI�����ꂽ�Z���̂ݕ`��(1�ŐV�����Z����I���C2�ł��łɑI������Ă���Z����`��C0�ł��ׂẴZ����`��)
 */
void rxPBDSPH_GPU::DrawCells(Vec3 col, Vec3 col2, int sel)
{
	glPushMatrix();

	uint *hCellStart     = new uint[m_dCellData.uNumCells];
	uint *hPolyCellStart = new uint[m_dCellData.uNumCells];
	CuCopyArrayFromDevice(hCellStart,     m_dCellData.dCellStart,     0, 0, m_dCellData.uNumCells*sizeof(uint));
	CuCopyArrayFromDevice(hPolyCellStart, m_dCellData.dPolyCellStart, 0, 0, m_dCellData.uNumCells*sizeof(uint));

	if(sel){
		uint *hCellEnd     = new uint[m_dCellData.uNumCells];
		uint *hSortedIndex = new uint[m_uNumParticles];
		CuCopyArrayFromDevice(hCellEnd,     m_dCellData.dCellEnd,     0, 0, m_dCellData.uNumCells*sizeof(uint));
		CuCopyArrayFromDevice(hSortedIndex, m_dCellData.dSortedIndex, 0, 0, m_uNumParticles*sizeof(uint));

		// �����_���ɑI�񂾃Z���Ƃ��̒��̃p�[�e�B�N���̂ݕ`��
		static int grid_hash = 0;
		static uint start_index = 0xffffffff;
		if(sel == 1){
			do{
				grid_hash = RXFunc::Nrand(m_dCellData.uNumCells-1);
				start_index = hCellStart[grid_hash];
			}while(start_index == 0xffffffff);
		}

		uint w = grid_hash%(m_params.GridSize.x*m_params.GridSize.y);
		DrawCell(w%m_params.GridSize.x, w/m_params.GridSize.x, grid_hash/(m_params.GridSize.x*m_params.GridSize.y));

		glColor3d(1.0, 0.0, 0.0);
		glPointSize(10.0);
		glBegin(GL_POINTS);

		int c = 0;
		uint end_index = hCellEnd[grid_hash];
		for(uint j = start_index; j < end_index; ++j){
			uint idx = hSortedIndex[j];
			Vec3 pos;
			pos[0] = m_hPos[4*idx+0];
			pos[1] = m_hPos[4*idx+1];
			pos[2] = m_hPos[4*idx+2];
			
			glVertex3dv(pos);

			c++;
		}
		glEnd();
		cout << "cell(" << grid_hash << ") : " << c << endl;

		delete [] hCellEnd;
		delete [] hSortedIndex;
	}
	else{
		int cnt = 0;
		// �p�[�e�B�N�� or �|���S�����܂ޑS�Z���̕`��
		RXFOR3(0, (int)m_params.GridSize.x, 0, (int)m_params.GridSize.y, 0, (int)m_params.GridSize.z){
			bool disp = false;
			uint grid_hash = calGridHash(i, j, k);
			uint start_index = hCellStart[grid_hash];
			uint start_index_poly = 0xffffffff;
		
			if(m_dCellData.uNumPolyHash) start_index_poly = hPolyCellStart[grid_hash];

			if(start_index != 0xffffffff){
				glColor3dv(col2.data);
				disp = true;
			}
			if(start_index_poly != 0xffffffff){
				glColor3dv(col.data);
				disp = true;
			}

			cnt++;

			if(disp){
				DrawCell(i, j, k);
			}
		}

		cout << cnt << endl;
	}

	delete [] hCellStart;
	delete [] hPolyCellStart;

	glPopMatrix();
}

/*!
 * �ő̏�Q���̕`��
 */
void rxPBDSPH_GPU::DrawObstacles(int drw)
{
#if MAX_BOX_NUM
	for(int i = 0; i < (int)m_params.BoxNum; ++i){
		if(!m_params.BoxFlg[i]) continue;

		Vec3 bcen = Vec3(m_params.BoxCen[i].x, m_params.BoxCen[i].y, m_params.BoxCen[i].z);
		Vec3 bext = Vec3(m_params.BoxExt[i].x, m_params.BoxExt[i].y, m_params.BoxExt[i].z);
		float bmat[16];
		GetGLMatrix(m_params.BoxRot[i], bmat);

		glPushMatrix();
		glTranslated(bcen[0], bcen[1], bcen[2]);
		glMultMatrixf(bmat);
		glScalef(2.0*bext[0], 2.0*bext[1], 2.0*bext[2]);
		//glRotated(brot, 0, 0, 1);
		glutWireCube(1.0);
		glPopMatrix();
	}
#endif

#if MAX_SPHERE_NUM
	for(int i = 0; i < (int)m_params.SphereNum; ++i){
		if(!m_params.SphereFlg[i]) continue;

		Vec3 scen  = Vec3(m_params.SphereCen[i].x, m_params.SphereCen[i].y, m_params.SphereCen[i].z);
		float srad = m_params.SphereRad[i];

		glPushMatrix();
		glTranslated(scen[0], scen[1], scen[2]);
		glutWireSphere(srad, 32, 32);
		glPopMatrix();
	}
#endif

	// �|���S�����b�V���̕`��
	glEnable(GL_LIGHTING);
	glColor4d(0.0, 1.0, 0.0, 1.0);
	m_Poly.Draw(drw & 14);
}





/*!
 * �O�p�`�|���S���ɂ���Q��
 * @param[in] vrts ���_
 * @param[in] tris ���b�V��
 */
void rxPBDSPH_GPU::SetPolygonObstacle(const vector<Vec3> &vrts, const vector<Vec3> &nrms, const vector< vector<int> > &tris, Vec3 vel)
{
	int vn = (int)vrts.size();
	int n = (int)tris.size();
	m_iNumVrts += vn;
	m_iNumTris += n;

	for(int i = 0; i < vn; ++i){
		for(int j = 0; j < 3; ++j){
			m_vVrts.push_back(vrts[i][j]);
		}
	}

	for(int i = 0; i < n; ++i){
		for(int j = 0; j < 3; ++j){
			m_vTris.push_back(tris[i][j]);
		}
	}

	// GPU�������̊m�ۂƓ]��
	if(m_dVrts) CuFreeArray(m_dVrts);
	if(m_dTris) CuFreeArray(m_dTris);
	m_dVrts = 0;
	m_dTris = 0;

	CuAllocateArray((void**)&m_dVrts, m_iNumVrts*3*sizeof(RXREAL));
	CuAllocateArray((void**)&m_dTris, m_iNumTris*3*sizeof(int));

	CuCopyArrayToDevice(m_dVrts, &m_vVrts[0], 0, m_iNumVrts*3*sizeof(RXREAL));
	CuCopyArrayToDevice(m_dTris, &m_vTris[0], 0, m_iNumTris*3*sizeof(int));

	RXCOUT << "the number of triangles : " << m_iNumTris << endl;

	setPolysToCell(&m_vVrts[0], m_iNumVrts, &m_vTris[0], m_iNumTris);
}




/*!
 * �O�p�`�|���S���ɂ��ő̃I�u�W�F�N�g�̒ǉ�
 * @param[in] filename �|���S���t�@�C����(OBJ,WRL�Ȃ�)
 * @param[in] cen �ő̃I�u�W�F�N�g���S���W
 * @param[in] ext �ő̃I�u�W�F�N�g�̑傫��(�ӂ̒�����1/2)
 * @param[in] ang �ő̃I�u�W�F�N�g�̊p�x(�I�C���[�p)
 * @param[in] vel �ő̃I�u�W�F�N�g�̑��x
 */
void rxPBDSPH_GPU::SetPolygonObstacle(const string &filename, Vec3 cen, Vec3 ext, Vec3 ang, Vec3 vel)
{
	// �|���S��������
	if(!m_Poly.vertices.empty()){
		m_Poly.vertices.clear();
		m_Poly.normals.clear();
		m_Poly.faces.clear();
		m_Poly.materials.clear();
	}
	string extension = GetExtension(filename);
	if(extension == "obj"){
		rxOBJ obj;
		if(obj.Read(filename, m_Poly.vertices, m_Poly.normals, m_Poly.faces, m_Poly.materials, true)){
			RXCOUT << filename << " have been read." << endl;

			RXCOUT << " the number of vertex   : " << m_Poly.vertices.size() << endl;
			RXCOUT << " the number of normal   : " << m_Poly.normals.size() << endl;
			RXCOUT << " the number of polygon  : " << m_Poly.faces.size() << endl;
			RXCOUT << " the number of material : " << m_Poly.materials.size() << endl;

			m_Poly.open = 1;

			// �|���S�����_��AABB�Ƀt�B�b�g
			if(RXFunc::IsZeroVec(ang)){
				fitVertices(cen, ext, m_Poly.vertices);
			}
			else{
				AffineVertices(m_Poly, cen, ext, ang);
			}


			// �|���S�����_�@���̌v�Z
			if(m_Poly.normals.empty()){
				CalVertexNormals(m_Poly);
			}


			//
			// �T���p�O���b�h�Ƀ|���S�����i�[
			//
			vector< vector<int> > tris;
			int pn = m_Poly.faces.size();
			tris.resize(pn);
			for(int i = 0; i < pn; ++i){
				tris[i].resize(3);
				for(int j = 0; j < 3; ++j){
					tris[i][j] = m_Poly.faces[i][j];
				}
			}
			SetPolygonObstacle(m_Poly.vertices, m_Poly.normals, tris, vel);

			//// ���C�g���Ȃǂł̕`��p�Ɍő̃I�u�W�F�N�g���b�V�����t�@�C���ۑ����Ă���
			//string outfn = RX_DEFAULT_MESH_DIR+"solid_boundary.obj";
			//rxOBJ saver;
			//rxMTL mtl;	// �ގ���empty�̂܂�
			//if(saver.Save(outfn, m_Poly.vertices, m_Poly.normals, m_Poly.faces, mtl)){
			//	RXCOUT << "saved the mesh to " << outfn << endl;
			//}

			// �ő̃p�[�e�B�N�������p��CPU���ɂ������i�[���Ă���
			rxSolidPolygon *obj = new rxSolidPolygon(filename, cen, ext, ang, m_params.EffectiveRadius, 1);
			//double m[16];
			//EulerToMatrix(m, ang[0], ang[1], ang[2]);
			//obj->SetMatrix(m);

			m_vSolids.push_back(obj);


		}
	}
}

/*!
 * ���_���AABB�ɍ����悤��Fit������(���̌`��̏c����͈ێ�)
 * @param[in] ctr AABB���S���W
 * @param[in] sl  AABB�̕ӂ̒���(1/2)
 * @param[in] vec_set ���_��
 * @param[in] start_index,end_index ���_��̌����͈�
 */
bool rxPBDSPH_GPU::fitVertices(const Vec3 &ctr, const Vec3 &sl, vector<Vec3> &vec_set)
{
	if(vec_set.empty()) return false;

	int n = (int)vec_set.size();

	// ���݂�BBox�̑傫���𒲂ׂ�
	Vec3 maxp = vec_set[0];
	Vec3 minp = vec_set[0];

	for(int i = 1; i < n; ++i){
		if(vec_set[i][0] > maxp[0]) maxp[0] = vec_set[i][0];
		if(vec_set[i][1] > maxp[1]) maxp[1] = vec_set[i][1];
		if(vec_set[i][2] > maxp[2]) maxp[2] = vec_set[i][2];
		if(vec_set[i][0] < minp[0]) minp[0] = vec_set[i][0];
		if(vec_set[i][1] < minp[1]) minp[1] = vec_set[i][1];
		if(vec_set[i][2] < minp[2]) minp[2] = vec_set[i][2];
	}

	Vec3 ctr0, sl0;
	sl0  = (maxp-minp)/2.0;
	ctr0 = (maxp+minp)/2.0;

	int max_axis = ( ( (sl0[0] > sl0[1]) && (sl0[0] > sl0[2]) ) ? 0 : ( (sl0[1] > sl0[2]) ? 1 : 2 ) );
	int min_axis = ( ( (sl0[0] < sl0[1]) && (sl0[0] < sl0[2]) ) ? 0 : ( (sl0[1] < sl0[2]) ? 1 : 2 ) );
	Vec3 size_conv = Vec3(sl[max_axis]/sl0[max_axis]);

	// �S�Ă̒��_��bbox�ɂ��킹�ĕϊ�
	for(int i = 0; i < n; ++i){
		vec_set[i] = (vec_set[i]-ctr0)*size_conv+ctr;
	}

	return true;
}



/*!
 * �{�b�N�X�^��Q��
 * @param[in] cen �{�b�N�X���S���W
 * @param[in] ext �{�b�N�X�̑傫��(�ӂ̒�����1/2)
 * @param[in] ang �{�b�N�X�̊p�x(�I�C���[�p)
 * @param[in] flg �L��/�����t���O
 */
void rxPBDSPH_GPU::SetBoxObstacle(Vec3 cen, Vec3 ext, Vec3 ang, Vec3 vel, int flg)
{
	// ��Q��
	int b = m_params.BoxNum;

	if(b < MAX_BOX_NUM){
		m_params.BoxCen[b] = MAKE_FLOAT3(cen[0], cen[1], cen[2]);
		m_params.BoxExt[b] = MAKE_FLOAT3(ext[0], ext[1], ext[2]);
		m_params.BoxRot[b] = EulerToMatrix(ang[0], ang[1], ang[2]);
		m_params.BoxInvRot[b] = Inverse(m_params.BoxRot[b]);
		m_params.BoxFlg[b] = flg;
		b++;

		m_params.BoxNum = b;
	}
}

/*!
 * ���^��Q��
 * @param[in] cen ���̒��S���W
 * @param[in] rad ���̂̔��a
 * @param[in] flg �L��/�����t���O
 */
void rxPBDSPH_GPU::SetSphereObstacle(Vec3 cen, double rad, Vec3 vel, int flg)
{
	// ��Q��
	int b = m_params.SphereNum;

	if(b < MAX_SPHERE_NUM){
		m_params.SphereCen[b] = MAKE_FLOAT3(cen[0], cen[1], cen[2]);
		m_params.SphereRad[b] = (RXREAL)rad;
		m_params.SphereFlg[b] = flg;
		b++;

		m_params.SphereNum = b;
	}
}


/*!
 * VBO,�f�o�C�X����������z�X�g�������փf�[�^��]���C�擾
 * @param[in] type �f�[�^�̎��
 * @return �z�X�g��������̃f�[�^
 */
RXREAL* rxPBDSPH_GPU::GetArrayVBO(rxParticleArray type, bool d2h, int num)
{
	assert(m_bInitialized);
 
	if(num == -1) num = m_uNumParticles;

	RXREAL* hdata = 0;
	RXREAL* ddata = 0;

	cudaGraphicsResource **graphics_resource = 0;
	int d = DIM;

	switch(type){
	default:
	case RX_POSITION:
		hdata = m_hPos;
		ddata = m_dPos;
		graphics_resource = &m_pPosResource;
		break;

	case RX_VELOCITY:
		hdata = m_hVel;
		ddata = m_dVel;
		break;

	case RX_DENSITY:
		hdata = m_hDens;
		ddata = m_dDens;
		d = 1;
		break;

	case RX_FORCE:
		hdata = m_hFrc;
		ddata = m_dFrc;
		break;

	case RX_PREDICT_POS:
		hdata = m_hPredictPos;
		ddata = m_dPredictPos;
		break;

	case RX_PREDICT_VEL:
		hdata = m_hPredictVel;
		ddata = m_dPredictVel;
		break;

	case RX_SCALING_FACTOR:
		hdata = m_hS;
		ddata = m_dS;
		d = 1;
		break;

	case RX_CORRECTION:
		hdata = m_hDp;
		ddata = m_dDp;
		break;

	case RX_BOUNDARY_PARTICLE:
		hdata = m_hPosB;
		ddata = m_dPosB;
		break;

	case RX_BOUNDARY_PARTICLE_VOL:
		hdata = m_hVolB;
		ddata = m_dVolB;
		d = 1;
		break;
	}

	if(d2h) CuCopyArrayFromDevice(hdata, ddata, graphics_resource, 0, num*d*sizeof(RXREAL));

	return hdata;
}

/*!
 * �z�X�g����������VBO,�f�o�C�X�������փf�[�^��]��
 * @param[in] type �f�[�^�̎��
 * @param[in] data �z�X�g��������̃f�[�^
 * @param[in] start �f�[�^�̊J�n�C���f�b�N�X
 * @param[in] count �ǉ���
 */
void rxPBDSPH_GPU::SetArrayVBO(rxParticleArray type, const RXREAL* data, int start, int count)
{
	assert(m_bInitialized);
 
	switch(type){
	default:
	case RX_POSITION:
		{
			if(m_bUseOpenGL){
				CuUnregisterGLBufferObject(m_pPosResource);
				glBindBuffer(GL_ARRAY_BUFFER, m_posVBO);
				glBufferSubData(GL_ARRAY_BUFFER, start*4*sizeof(RXREAL), count*4*sizeof(RXREAL), data);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				CuRegisterGLBufferObject(m_posVBO, &m_pPosResource);
			}
		}
		break;

	case RX_VELOCITY:
		CuCopyArrayToDevice(m_dVel, data, start*DIM*sizeof(RXREAL), count*DIM*sizeof(RXREAL));
		break;

	case RX_BOUNDARY_PARTICLE:
		CuCopyArrayToDevice(m_dPosB, data, start*DIM*sizeof(RXREAL), count*DIM*sizeof(RXREAL));
		break;
	}	   
}


//-----------------------------------------------------------------------------
// �A�֐��l
//-----------------------------------------------------------------------------
double rxPBDSPH_GPU::GetImplicit(double x, double y, double z)
{
	return 0.0;
}

/*!
 * �p�[�e�B�N������O���b�h�̉A�֐��l���v�Z
 * @param[in] pnx,pny,pnz �O���b�h���̎w�� nx=2^pnx
 * @param[in] minp �O���b�h�̍ŏ����W
 * @param[in] d �O���b�h��
 * @param[out] hF �A�֐��l(nx�~ny�~nz�̔z��)
 */
void rxPBDSPH_GPU::CalImplicitField(int n[3], Vec3 minp, Vec3 d, RXREAL *hF)
{
	unsigned int memSize = sizeof(RXREAL)*n[0]*n[1]*n[2];
	//RXCOUT << memSize/sizeof(RXREAL) << endl;
	
	RXREAL *dF = 0;
	CuAllocateArray((void**)&dF, memSize);

	RXREAL *dPos;
	if(m_bUseOpenGL){
		dPos = (RXREAL*)CuMapGLBufferObject(&m_pPosResource);
	}
	else{
		dPos = (RXREAL*)m_dPos;
	}

	// �����Z���̃n�b�V�����v�Z
	CuCalcHash(m_dCellData.dGridParticleHash, m_dCellData.dSortedIndex, dPos, m_uNumParticles);

	// �n�b�V���Ɋ�Â��p�[�e�B�N�����\�[�g
	CuSort(m_dCellData.dGridParticleHash, m_dCellData.dSortedIndex, m_uNumParticles);

	// �p�[�e�B�N���z����\�[�g���ꂽ���Ԃɕ��ёւ��C
	// �e�Z���̎n�܂�ƏI���̃C���f�b�N�X������
	CuReorderDataAndFindCellStart(m_dCellData, dPos, m_dVel);

	// �p�[�e�B�N�����x��p�����{�����[���f�[�^
	CuPbfGridDensity(dF, m_dCellData, n[0], n[1], n[2], minp[0], minp[1], minp[2], d[0], d[1], d[2]);

	if(m_bUseOpenGL){
		CuUnmapGLBufferObject(m_pPosResource);
	}

	CuCopyArrayFromDevice(hF, dF, 0, 0, memSize);

	if(dF) CuFreeArray(dF);
}


/*!
 * �p�[�e�B�N������O���b�h�̉A�֐��l���v�Z
 * @param[in] n[3] �O���b�h��
 * @param[in] minp �O���b�h�̍ŏ����W
 * @param[in] d �O���b�h��
 * @param[out] hF �A�֐��l(nx�~ny�~nz�̔z��)
 */
void rxPBDSPH_GPU::CalImplicitFieldDevice(int n[3], Vec3 minp, Vec3 d, RXREAL *dF)
{
	RXREAL *dPos;
	if(m_bUseOpenGL){
		dPos = (RXREAL*)CuMapGLBufferObject(&m_pPosResource);
	}
	else{
		dPos = (RXREAL*)m_dPos;
	}

	// �����Z���̃n�b�V�����v�Z
	CuCalcHash(m_dCellData.dGridParticleHash, m_dCellData.dSortedIndex, dPos, m_uNumParticles);

	// �n�b�V���Ɋ�Â��p�[�e�B�N�����\�[�g
	CuSort(m_dCellData.dGridParticleHash, m_dCellData.dSortedIndex, m_uNumParticles);

	// �p�[�e�B�N���z����\�[�g���ꂽ���Ԃɕ��ёւ��C
	// �e�Z���̎n�܂�ƏI���̃C���f�b�N�X������
	CuReorderDataAndFindCellStart(m_dCellData, dPos, m_dVel);

	// �p�[�e�B�N�����x��p�����{�����[���f�[�^
	CuPbfGridDensity(dF, m_dCellData, n[0], n[1], n[2], minp[0], minp[1], minp[2], d[0], d[1], d[2]);

	if(m_bUseOpenGL){
		CuUnmapGLBufferObject(m_pPosResource);
	}
}





//-----------------------------------------------------------------------------
// MARK:�V�~���f�[�^�̏o��
//-----------------------------------------------------------------------------
/*!
 * �V�~�����[�V�����ݒ�(�p�[�e�B�N�����C�͈́C���x�C���ʂȂ�)
 * @param[in] fn �o�̓t�@�C����
 */
void rxPBDSPH_GPU::OutputSetting(string fn)
{
	ofstream fout;
	fout.open(fn.c_str());
	if(!fout){
		RXCOUT << fn << " couldn't open." << endl;
		return;
	}

	fout << m_uNumParticles << endl;
	fout << m_params.Boundary[0].x << " " << m_params.Boundary[0].y << " " << m_params.Boundary[0].z << endl;
	fout << m_params.Boundary[1].x << " " << m_params.Boundary[1].y << " " << m_params.Boundary[1].z << endl;
	fout << m_params.Density << endl;
	fout << m_params.Mass    << endl;
	fout << m_params.KernelParticles << endl;

	fout.close();
}
