/*! 
  @file rx_cu_common.cuh
	
  @brief CUDA���ʃw�b�_
 
  @author Makoto Fujisawa
  @date 2009-08, 2011-06
*/
// FILE --rx_cu_common.cuh--

#ifndef _RX_CU_COMMON_CUH_
#define _RX_CU_COMMON_CUH_


//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
#include "vector_types.h"
#include "vector_functions.h"


//-----------------------------------------------------------------------------
// ��`
//-----------------------------------------------------------------------------


typedef unsigned int uint;
typedef unsigned char uchar;

#define FLOAT float
#define FLOAT3 float3

#define MAKE_FLOAT3 make_float3

#define RX_CUMC_USE_GEOMETRY

#define RX_USE_ATOMIC_FUNC // �vCompute capability 1.1�ȏ�(-arch sm_11)


// �e�N�X�`���������̎g�p�t���O
#ifndef __DEVICE_EMULATION__
#define USE_TEX 0
#endif

#if USE_TEX
#define FETCH(t, i) tex1Dfetch(t##Tex, i)
#else
#define FETCH(t, i) t[i]
#endif

#if USE_TEX
#define FETCHC(t, i) tex1Dfetch(t##Tex, i)
#else
#define FETCHC(t, i) cell.t[i]
#endif


// 1�u���b�N������̃X���b�h��(/����)
#define BLOCK_SIZE 16

// 1�u���b�N������̍ő�X���b�h��
#define THREAD_NUM 512

// �T���v���{�����[����p����ꍇ1, �A�֐���p����ꍇ��0
#define SAMPLE_VOLUME 1

// Shared Memory�̎g�p�t���O
#define USE_SHARED 1

// Shared Memory��p����ꍇ�̃X���b�h���̐���
#define NTHREADS 32

#define SKIP_EMPTY_VOXELS 1



// �s��
struct matrix3x3
{
	float3 e[3];
};

struct matrix4x4
{
	float4 e[4];
};


#define MAX_POLY_NUM 10
#define MAX_BOX_NUM 10
#define MAX_SPHERE_NUM 10

#define DEBUG_N 17


//-----------------------------------------------------------------------------
//! SPH�V�~�����[�V�����p�����[�^
//-----------------------------------------------------------------------------
struct rxSimParams
{
	FLOAT3 Gravity;
	FLOAT ParticleRadius;

	float3 Boundary[2];

	uint3 GridSize;
	uint NumCells;
	FLOAT3 WorldOrigin;
	FLOAT3 WorldMax;
	FLOAT3 CellWidth;

	uint NumBodies;
	uint MaxParticlesPerCell;

	uint3 GridSizeB;
	uint NumCellsB;
	FLOAT3 WorldOriginB;
	FLOAT3 WorldMaxB;
	FLOAT3 CellWidthB;
	uint NumBodiesB;


	FLOAT EffectiveRadius;
	FLOAT Mass;			// �p�[�e�B�N������[kg]
	FLOAT VorticityConfinement;

	FLOAT Buoyancy;

	FLOAT Density;		// ���x[kg/m^3]
	FLOAT Pressure;     // [Pa = N/m^2 = kg/m.s^2]

	FLOAT Tension;		// [N/m = kg/s^2]
	FLOAT Viscosity;	// [Pa.s = N.s/m^2 = kg/m.s]
	FLOAT GasStiffness;	// [J = N.m = kg.m^2/s^2]  // used for DC96 symmetric pressure force

	FLOAT Volume;
	FLOAT KernelParticles;
	FLOAT Restitution;

	FLOAT Threshold;

	FLOAT InitDensity;

	FLOAT Dt;

	FLOAT Wpoly6;		//!< Pory6�J�[�l���̒萔�W��
	FLOAT GWpoly6;		//!< Pory6�J�[�l���̌��z�̒萔�W��
	FLOAT LWpoly6;		//!< Pory6�J�[�l���̃��v���V�A���̒萔�W��
	FLOAT Wspiky;		//!< Spiky�J�[�l���̒萔�W��
	FLOAT GWspiky;		//!< Spiky�J�[�l���̌��z�̒萔�W��
	FLOAT LWspiky;		//!< Spiky�J�[�l���̃��v���V�A���̒萔�W��
	FLOAT Wvisc;		//!< Viscosity�J�[�l���̒萔�W��
	FLOAT GWvisc;		//!< Viscosity�J�[�l���̌��z�̒萔�W��
	FLOAT LWvisc;		//!< Viscosity�J�[�l���̃��v���V�A���̒萔�W��

	FLOAT Wpoly6r;		//!< Pory6�J�[�l���̒萔�W��(�ő�l��1�ɂȂ�悤�ɐ��K�������W��)

	int AP;
	FLOAT AP_K;
	FLOAT AP_N;
	FLOAT AP_Q;
	FLOAT AP_WQ;

	uint   PolyNum;
	FLOAT3 PolyVel[MAX_POLY_NUM];

	uint   BoxNum;
#if MAX_BOX_NUM
	FLOAT3 BoxCen[MAX_BOX_NUM];
	FLOAT3 BoxExt[MAX_BOX_NUM];
	//FLOAT3 BoxVel[MAX_BOX_NUM];
	matrix3x3 BoxRot[MAX_BOX_NUM];
	matrix3x3 BoxInvRot[MAX_BOX_NUM];
	uint   BoxFlg[MAX_BOX_NUM];
#endif

	uint SphereNum;
#if MAX_SPHERE_NUM
	FLOAT3 SphereCen[MAX_SPHERE_NUM];
	FLOAT  SphereRad[MAX_SPHERE_NUM];
	//FLOAT3 SphereVel[MAX_SPHERE_NUM];
	uint   SphereFlg[MAX_SPHERE_NUM];
#endif
};



struct rxParticleCell
{
	float4* dSortedPos;			//!< �\�[�g�ς݃p�[�e�B�N�����W
	float4* dSortedVel;			//!< �\�[�g�ς݃p�[�e�B�N�����x

	uint* dSortedIndex;			//!< �\�[�g�ς݃p�[�e�B�N���C���f�b�N�X
	uint* dGridParticleHash;	//!< �e�p�[�e�B�N���̃O���b�h�n�b�V���l(�\�[�g�p�L�[)
	uint* dCellStart;			//!< �\�[�g���X�g���̊e�Z���̃X�^�[�g�C���f�b�N�X
	uint* dCellEnd;				//!< �\�[�g���X�g���̊e�Z���̃G���h�C���f�b�N�X
	uint  uNumParticles;		//!< ���p�[�e�B�N����
	uint  uNumCells;			//!< ���Z����
	uint  uNumArdGrid;			//!< �ߖT�T�����Q�ƃO���b�h�͈�

	uint* dSortedPolyIdx;		//!< �\�[�g�ς݃|���S���C���f�b�N�X
	uint* dGridPolyHash;		//!< �|���S���̃O���b�h�n�b�V���l(�\�[�g�p�L�[)
	uint* dPolyCellStart;		//!< �\�[�g���X�g���̊e�Z���̃X�^�[�g�C���f�b�N�X
	uint* dPolyCellEnd;			//!< �\�[�g���X�g���̊e�Z���̃G���h�C���f�b�N�X
	uint  uNumPolyHash;
};





#endif // _RX_CU_COMMON_CUH_
