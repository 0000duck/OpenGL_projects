/*! 
  @file rx_sph.cu
	
  @brief CUDA�ɂ��SPH

  @author Makoto Fujisawa
  @date 2009-08, 2011-06
*/
// FILE --rx_sph.cu--


//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
#include <cstdio>
#include <GL/glew.h>
#include <GL/glut.h>

#include "rx_pbf_kernel.cu"

#include <thrust/device_vector.h>
#include <thrust/scan.h>



//-----------------------------------------------------------------------------
// CUDA�֐�
//-----------------------------------------------------------------------------
extern "C"
{
	
void CuSetParameters(rxSimParams *hostParams)
{
	// copy parameters to constant memory
	RX_CUCHECK( cudaMemcpyToSymbol(params, hostParams, sizeof(rxSimParams)) );
}

void CuClearData(void)
{
}

/*!
 * thrust::exclusive_scan�̌Ăяo��
 * @param[out] dScanData scan��̃f�[�^
 * @param[in] dData ���f�[�^
 * @param[in] num �f�[�^��
 */
void CuScanf(float* dScanData, float* dData, unsigned int num)
{
	thrust::exclusive_scan(thrust::device_ptr<float>(dData), 
						   thrust::device_ptr<float>(dData+num),
						   thrust::device_ptr<float>(dScanData));
}


/*!
 * �����Z���̃n�b�V�����v�Z
 * @param[in] 
 * @return 
 */
void CuCalcHash(uint* dGridParticleHash, uint* dSortedIndex, float* dPos, int nprts)
{
	uint numThreads, numBlocks;
	computeGridSize(nprts, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	calcHashD<<< numBlocks, numThreads >>>(dGridParticleHash,
										   dSortedIndex,
										   (float4*)dPos,
										   nprts);
	
	RX_CUERROR("calcHashD kernel execution failed");	// �J�[�l���G���[�`�F�b�N
}


/*!
 * �p�[�e�B�N���z����\�[�g���ꂽ���Ԃɕ��ёւ��C
 * �e�Z���̎n�܂�ƏI���̃C���f�b�N�X������
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 * @param[in] oldPos �p�[�e�B�N���ʒu
 * @param[in] oldVel �p�[�e�B�N�����x
 */
void CuReorderDataAndFindCellStart(rxParticleCell cell, float* oldPos, float* oldVel)
{
	uint numThreads, numBlocks;
	computeGridSize(cell.uNumParticles, THREAD_NUM, numBlocks, numThreads);

	RX_CUCHECK(cudaMemset(cell.dCellStart, 0xffffffff, cell.uNumCells*sizeof(uint)));

	uint smemSize = sizeof(uint)*(numThreads+1);

	// �J�[�l�����s
	reorderDataAndFindCellStartD<<< numBlocks, numThreads, smemSize>>>(cell, (float4*)oldPos, (float4*)oldVel);

	RX_CUERROR("reorderDataAndFindCellStartD kernel execution failed");
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}


/*!
 * �����Z���̃n�b�V�����v�Z
 * @param[in] 
 * @return 
 */
void CuCalcHashB(uint* dGridParticleHash, uint* dSortedIndex, float* dPos, 
				 float3 world_origin, float3 cell_width, uint3 grid_size, int nprts)
{
	uint numThreads, numBlocks;
	computeGridSize(nprts, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	calcHashB<<< numBlocks, numThreads >>>(dGridParticleHash,
										   dSortedIndex,
										   (float4*)dPos,
										   world_origin, 
										   cell_width, 
										   grid_size, 
										   nprts);
	
	RX_CUERROR("Kernel execution failed : calcHashB");	// �J�[�l���G���[�`�F�b�N
}

/*!
 * �p�[�e�B�N���z����\�[�g���ꂽ���Ԃɕ��ёւ��C
 * �e�Z���̎n�܂�ƏI���̃C���f�b�N�X������
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 * @param[in] oldPos �p�[�e�B�N���ʒu
 */
void CuReorderDataAndFindCellStartB(rxParticleCell cell, float* oldPos)
{
	uint numThreads, numBlocks;
	computeGridSize(cell.uNumParticles, THREAD_NUM, numBlocks, numThreads);

	RX_CUCHECK(cudaMemset(cell.dCellStart, 0xffffffff, cell.uNumCells*sizeof(uint)));

	uint smemSize = sizeof(uint)*(numThreads+1);

	// �J�[�l�����s
	reorderDataAndFindCellStartB<<< numBlocks, numThreads, smemSize>>>(cell, (float4*)oldPos);

	RX_CUERROR("Kernel execution failed: CuReorderDataAndFindCellStartB");
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}










//-----------------------------------------------------------------------------
// ���E�p�[�e�B�N������
//-----------------------------------------------------------------------------
/*!
 * ���E�p�[�e�B�N���̑̐ς��v�Z
 *  - "Versatile Rigid-Fluid Coupling for Incompressible SPH", 2.2 ��(3)�̏�
 * @param[out] dVolB ���E�p�[�e�B�N���̑̐�
 * @param[in]  mass �p�[�e�B�N������
 * @param[in]  cell �p�[�e�B�N���O���b�h�f�[�^
 */
void CuSphBoundaryVolume(float* dVolB, float mass, rxParticleCell cell)
{
	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(cell.uNumParticles, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	sphCalBoundaryVolume<<< numBlocks, numThreads >>>(dVolB, cell);

	RX_CUERROR("kernel execution failed : sphCalBoundaryVolume");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}

/*!
 * �p�[�e�B�N�����x�̌v�Z(�J�[�l���Ăяo��)
 * @param[out] dDens �p�[�e�B�N�����x
 * @param[out] dPres �p�[�e�B�N������
 * @param[in]  cell �p�[�e�B�N���O���b�h�f�[�^
 */
void CuSphBoundaryDensity(float* dDens, float* dPres, float* dPos, float* dVolB, rxParticleCell bcell, uint pnum)
{
	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(pnum, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	sphCalBoundaryDensity<<< numBlocks, numThreads >>>(dDens, dPres, (float4*)dPos, dVolB, bcell, pnum);

	RX_CUERROR("kernel execution failed : sphCalBoundaryDensity");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}

/*!
 * �p�[�e�B�N���ɂ�����͂̌v�Z(�J�[�l���Ăяo��)
 * @param[in] dDens �p�[�e�B�N�����x
 * @param[in] dPres �p�[�e�B�N������
 * @param[out] dFrc �p�[�e�B�N���ɂ������
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 * @param[in] dt ���ԃX�e�b�v��
 */
void CuSphBoundaryForces(float* dDens, float* dPres, float* dPos, float* dVolB, float* dFrc, rxParticleCell bcell, uint pnum)
{
	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(pnum, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	sphCalBoundaryForce<<< numBlocks, numThreads >>>(dDens, dPres, (float4*)dPos, dVolB, (float4*)dFrc, bcell, pnum);

	RX_CUERROR("kernel execution failed : sphCalBoundaryForce");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}





//-----------------------------------------------------------------------------
// PBF
//-----------------------------------------------------------------------------

/*!
 * �p�[�e�B�N�����x�̌v�Z(�J�[�l���Ăяo��)
 * @param[out] dDens �p�[�e�B�N�����x
 * @param[out] dPres �p�[�e�B�N������
 * @param[in]  cell �p�[�e�B�N���O���b�h�f�[�^
 */
void CuPbfDensity(float* dDens, rxParticleCell cell)
{
	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(cell.uNumParticles, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	pbfCalDensity<<< numBlocks, numThreads >>>(dDens, cell);

	RX_CUERROR("pbfCalDensity kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}

/*!
 * �p�[�e�B�N���ɂ�����͂̌v�Z(�J�[�l���Ăяo��)
 * @param[in] dDens �p�[�e�B�N�����x
 * @param[out] dFrc �p�[�e�B�N���ɂ������
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 * @param[in] dt ���ԃX�e�b�v��
 */
void CuPbfExternalForces(float* dDens, float* dFrc, rxParticleCell cell, float dt)
{
	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(cell.uNumParticles, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	pbfCalExternalForces<<< numBlocks, numThreads >>>(dDens, (float4*)dFrc, cell);

	RX_CUERROR("pbfCalExternalForces kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}

/*!
 * �X�P�[�����O�t�@�N�^�̌v�Z
 * @param[in] dPos �p�[�e�B�N�����S���W
 * @param[out] dDens �p�[�e�B�N�����x
 * @param[out] dScl �X�P�[�����O�t�@�N�^
 * @param[in] eps �ɘa�W��
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 */
void CuPbfScalingFactor(float* dPos, float* dDens, float* dScl, float eps, rxParticleCell cell)
{
	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(cell.uNumParticles, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	pbfCalScalingFactor<<< numBlocks, numThreads >>>((float4*)dPos, dDens, dScl, eps, cell);

	RX_CUERROR("pbfCalScalingFactor kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}

/*!
 * ���ϖ��x�ϓ��̌v�Z
 *  - ���ׂẴp�[�e�B�N�����x�̏������x�Ƃ̍����J�[�l���Ōv�Z���CPrefix Sum (Scan)�ł��̍��v�����߂�
 * @param[out] dErrScan �ϓ��l��Scan���ʂ��i�[����z��
 * @param[out] dErr �p�[�e�B�N�����x�ϓ��l
 * @param[in] dDens �p�[�e�B�N�����x
 * @param[in] rest_dens �������x
 * @param[in] nprts �p�[�e�B�N����
 * @return ���ϖ��x�ϓ�
 */
float CuPbfCalDensityFluctuation(float* dErrScan, float* dErr, float* dDens, float rest_dens, uint nprts)
{
	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(nprts, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	pbfDensityFluctuation<<< numBlocks, numThreads >>>(dErr, dDens, rest_dens, nprts);

	RX_CUERROR("pbfDensityFluctuation kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�

	// �e�p�[�e�B�N���̖��x�ϓ���Scan
	CuScanf(dErrScan, dErr, nprts);

	// Exclusive scan (�Ō�̗v�f��0�Ԗڂ���n-2�Ԗڂ܂ł̍��v�ɂȂ��Ă���)�Ȃ̂ŁC
	// Scan�O�z��̍Ō�(n-1�Ԗ�)�̗v�f�ƍ��v���邱�ƂŖ��x�ϓ��̍��v���v�Z
	float lval, lsval;
	RX_CUCHECK(cudaMemcpy((void*)&lval, (void*)(dErr+nprts-1), sizeof(float), cudaMemcpyDeviceToHost));
	RX_CUCHECK(cudaMemcpy((void*)&lsval, (void*)(dErrScan+nprts-1), sizeof(float), cudaMemcpyDeviceToHost));
	float dens_var = lval+lsval;

	return dens_var/(float)nprts;
}

/*!
 * �ʒu�C���ʂ̌v�Z
 * @param[in] dPos �p�[�e�B�N�����S���W
 * @param[in] dScl �X�P�[�����O�t�@�N�^
 * @param[out] dDp �ʒu�C����
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 */
void CuPbfPositionCorrection(float* dPos, float* dScl, float* dDp, rxParticleCell cell)
{
	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(cell.uNumParticles, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	pbfPositionCorrection<<< numBlocks, numThreads >>>((float4*)dPos, dScl, (float4*)dDp, cell);

	RX_CUERROR("pbfPositionCorrection kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}

/*!
 * �p�[�e�B�N���ʒu���X�V
 * @param[inout] dPos �p�[�e�B�N���ʒu
 * @param[in] dDp �ʒu�C����
 * @param[in] nprts �p�[�e�B�N����
 */
void CuPbfCorrectPosition(float* dPos, float* dDp, uint nprts)
{
	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(nprts, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	pbfCorrectPosition<<< numBlocks, numThreads >>>((float4*)dPos, (float4*)dDp, nprts);
	
	RX_CUERROR("pbfCorrectPosition kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}



/*!
 * ���E�p�[�e�B�N�����x���]���̃p�[�e�B�N�����x�ɉ�����
 * @param[inout] dDens ���̃p�[�e�B�N�����x
 * @param[in] dPos  ���̃p�[�e�B�N������
 * @param[in] dVolB ���E�p�[�e�B�N���̐�
 * @param[in] bcell ���E�p�[�e�B�N���O���b�h�f�[�^
 */
void CuPbfBoundaryDensity(float* dDens, float* dPos, float* dVolB, rxParticleCell bcell, uint pnum)
{
	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(pnum, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	pbfCalBoundaryDensity<<< numBlocks, numThreads >>>(dDens, (float4*)dPos, dVolB, bcell, pnum);

	RX_CUERROR("pbfCalBoundaryDensity kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}


/*!
 * �X�P�[�����O�t�@�N�^�̌v�Z(���E�p�[�e�B�N���܂�)
 * @param[in] dPos ���̃p�[�e�B�N�����S���W
 * @param[out] dDens ���̃p�[�e�B�N�����x
 * @param[out] dScl ���̃p�[�e�B�N���̃X�P�[�����O�t�@�N�^
 * @param[in] eps �ɘa�W��
 * @param[in] cell ���̃p�[�e�B�N���O���b�h�f�[�^
 * @param[in] dVolB ���E�p�[�e�B�N���̐�
 * @param[out] dSclB ���E�p�[�e�B�N���̃X�P�[�����O�t�@�N�^
 * @param[in] bcell ���E�p�[�e�B�N���O���b�h�f�[�^
 */
void CuPbfScalingFactorWithBoundary(float* dPos, float* dDens, float* dScl, float eps, rxParticleCell cell, 
									   float* dVolB, float* dSclB, rxParticleCell bcell)
{
	// ���̃p�[�e�B�N���̐������X���b�h�𗧂Ă�
	uint numThreads, numBlocks;
	computeGridSize(cell.uNumParticles, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	pbfCalScalingFactorWithBoundary<<< numBlocks, numThreads >>>((float4*)dPos, dDens, dScl, eps, cell, dVolB, bcell);

	RX_CUERROR("pbfCalScalingFactorWithBoundary kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�

	// ���E�p�[�e�B�N���̃X�P�[�����O�t�@�N�^�̌v�Z
	// ���E�p�[�e�B�N���̐������X���b�h�𗧂Ă�
	computeGridSize(bcell.uNumParticles, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	pbfCalBoundaryScalingFactor<<< numBlocks, numThreads >>>((float4*)dPos, dDens, eps, cell, dVolB, dSclB, bcell);

	RX_CUERROR("pbfCalScalingFactorWithBoundary kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�

}

/*!
 * �ʒu�C���ʂ̌v�Z(���E�p�[�e�B�N���܂�)
 * @param[in] dPos ���̃p�[�e�B�N�����S���W
 * @param[in] dScl ���̃p�[�e�B�N���̃X�P�[�����O�t�@�N�^
 * @param[out] dDens ���̃p�[�e�B�N���ʒu�C����
 * @param[in] cell ���̃p�[�e�B�N���O���b�h�f�[�^
 * @param[in] dVolB ���E�p�[�e�B�N���̐�
 * @param[in] dSclB ���E�p�[�e�B�N���̃X�P�[�����O�t�@�N�^
 * @param[in] bcell ���E�p�[�e�B�N���O���b�h�f�[�^
 */
void CuPbfPositionCorrectionWithBoundary(float* dPos, float* dScl, float* dDp, rxParticleCell cell, 
											float* dVolB, float* dSclB, rxParticleCell bcell)
{
	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(cell.uNumParticles, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	pbfPositionCorrectionWithBoundary<<< numBlocks, numThreads >>>((float4*)dPos, dScl, (float4*)dDp, cell, 
																	  dVolB, dSclB, bcell);

	RX_CUERROR("pbfPositionCorrectionWithBoundary kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}



/*!
 * �p�[�e�B�N���ʒu�C���x�̍X�V
 *  - �O�p�`�|���S�����E��
 *  - ���݂̈ʒu�̂ݎg���ďՓ˔���
 * @param[in] dPos �p�[�e�B�N���ʒu
 * @param[in] dVel �p�[�e�B�N�����x
 * @param[in] dAcc �p�[�e�B�N�������x
 * @param[out] dNewPos �X�V���ꂽ�p�[�e�B�N���ʒu
 * @param[out] dNewVel �X�V���ꂽ�p�[�e�B�N�����x
 * @param[in] dt ���ԃX�e�b�v��
 * @param[in] nprts �p�[�e�B�N����
 */
void CuPbfIntegrate(float* dPos, float* dVel, float* dAcc, 
					float* dNewPos, float* dNewVel, float dt, uint nprts)
{
	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(nprts, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	pbfIntegrate<<< numBlocks, numThreads >>>((float4*)dPos, (float4*)dVel, (float4*)dAcc, 
												 (float4*)dNewPos, (float4*)dNewVel, dt, nprts);
	
	RX_CUERROR("pbfIntegrate kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}

/*!
 * �p�[�e�B�N���ʒu�C���x�̍X�V
 *  - �O�p�`�|���S�����E��
 *  - ���݂̈ʒu�̂ݎg���ďՓ˔���
 * @param[in] dPos �p�[�e�B�N���ʒu
 * @param[in] dVel �p�[�e�B�N�����x
 * @param[in] dAcc �p�[�e�B�N�������x
 * @param[out] dNewPos �X�V���ꂽ�p�[�e�B�N���ʒu
 * @param[out] dNewVel �X�V���ꂽ�p�[�e�B�N�����x
 * @param[in] dVrts �O�p�`�|���S�����_
 * @param[in] dTris �O�p�`�|���S���C���f�b�N�X
 * @param[in] tri_num �O�p�`�|���S����
 * @param[in] dt ���ԃX�e�b�v��
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 */
void CuPbfIntegrateWithPolygon(float* dPos, float* dVel, float* dAcc, 
							   float* dNewPos, float* dNewVel, 
							   float* dVrts, int* dTris, int tri_num, float dt, rxParticleCell cell)
{
	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(cell.uNumParticles, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	pbfIntegrateWithPolygon<<< numBlocks, numThreads >>>((float4*)dPos, (float4*)dVel, (float4*)dAcc, 
															(float4*)dNewPos, (float4*)dNewVel, (float3*)dVrts, (int3*)dTris, tri_num, dt, cell);
	
	RX_CUERROR("pbfIntegrateWithPolygon kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}


/*!
 * �p�[�e�B�N���ʒu�C���x�̍X�V
 *  - ���݂̈ʒu�ƏC����̈ʒu��2���g���ďՓ˔���(PBF�������Ɏg�p)
 * @param[in] dPos �p�[�e�B�N���ʒu
 * @param[in] dVel �p�[�e�B�N�����x
 * @param[in] dAcc �p�[�e�B�N�������x
 * @param[out] dNewPos �X�V���ꂽ�p�[�e�B�N���ʒu
 * @param[out] dNewVel �X�V���ꂽ�p�[�e�B�N�����x
 * @param[in] dt ���ԃX�e�b�v��
 * @param[in] nprts �p�[�e�B�N����
 */
void CuPbfIntegrate2(float* dPos, float* dVel, float* dAcc, 
					 float* dNewPos, float* dNewVel, float dt, uint nprts)
{
	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(nprts, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	pbfIntegrate2<<< numBlocks, numThreads >>>((float4*)dPos, (float4*)dVel, (float4*)dAcc, 
												  (float4*)dNewPos, (float4*)dNewVel, dt, nprts);
	
	RX_CUERROR("pbfIntegrate2 kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}
/*!
 * �p�[�e�B�N���ʒu�C���x�̍X�V
 *  - �O�p�`�|���S�����E��
 *  - ���݂̈ʒu�ƏC����̈ʒu��2���g���ďՓ˔���(PBF�������Ɏg�p)
 * @param[in] dPos �p�[�e�B�N���ʒu
 * @param[in] dVel �p�[�e�B�N�����x
 * @param[in] dAcc �p�[�e�B�N�������x
 * @param[out] dNewPos �X�V���ꂽ�p�[�e�B�N���ʒu
 * @param[out] dNewVel �X�V���ꂽ�p�[�e�B�N�����x
 * @param[in] dVrts �O�p�`�|���S�����_
 * @param[in] dTris �O�p�`�|���S���C���f�b�N�X
 * @param[in] tri_num �O�p�`�|���S����
 * @param[in] dt ���ԃX�e�b�v��
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 */
void CuPbfIntegrateWithPolygon2(float* dPos, float* dVel, float* dAcc, 
								float* dNewPos, float* dNewVel, 
								float* dVrts, int* dTris, int tri_num, float dt, rxParticleCell cell)
{
	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(cell.uNumParticles, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	pbfIntegrateWithPolygon2<<< numBlocks, numThreads >>>((float4*)dPos, (float4*)dVel, (float4*)dAcc, 
															 (float4*)dNewPos, (float4*)dNewVel, (float3*)dVrts, (int3*)dTris, tri_num, dt, cell);
	
	RX_CUERROR("pbfIntegrateWithPolygon2 kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}


/*!
 * �p�[�e�B�N���ʒu�C���x�̍X�V
 * @param[in] pos �X�V���ꂽ�p�[�e�B�N���ʒu
 * @param[inout] new_pos �X�e�b�v�ŏ��̃p�[�e�B�N���ʒu/�V�����p�[�e�B�N�����x
 * @param[out] new_vel �V�����p�[�e�B�N�����x
 * @param[in] dt ���ԃX�e�b�v��
 * @param[in] nprts �p�[�e�B�N����
 */
void CuPbfUpdatePosition(float* dPos, float* dNewPos, float* dNewVel, float dt, uint nprts)
{
	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(nprts, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	pbfUpdatePosition<<< numBlocks, numThreads >>>((float4*)dPos, (float4*)dNewPos, (float4*)dNewVel, dt, nprts);
	
	RX_CUERROR("CupbfUpdatePosition kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}

/*!
 * �p�[�e�B�N���ʒu�C���x�̍X�V
 * @param[in] pos �X�V���ꂽ�p�[�e�B�N���ʒu
 * @param[inout] new_pos �X�e�b�v�ŏ��̃p�[�e�B�N���ʒu/�V�����p�[�e�B�N�����x
 * @param[out] new_vel �V�����p�[�e�B�N�����x
 * @param[in] dt ���ԃX�e�b�v��
 * @param[in] nprts �p�[�e�B�N����
 */
void CuPbfUpdateVelocity(float* dPos, float* dNewPos, float* dNewVel, float dt, uint nprts)
{
	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(nprts, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	pbfUpdateVelocity<<< numBlocks, numThreads >>>((float4*)dPos, (float4*)dNewPos, (float4*)dNewVel, dt, nprts);
	
	RX_CUERROR("pbfUpdateVelocity kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}

/*!
 * XSPH�ɂ��S���v�Z
 * @param[in] dPos �p�[�e�B�N�����S���W
 * @param[in] dVel �p�[�e�B�N�����x
 * @param[out] dNewVel �X�V���ꂽ�p�[�e�B�N�����x
 * @param[in] dDens �p�[�e�B�N�����x
 * @param[in] c �S���v�Z�p�p�����[�^
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 */
void CuXSphViscosity(float* dPos, float* dVel, float* dNewVel, float* dDens, float c, rxParticleCell cell)
{
	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(cell.uNumParticles, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	xsphVisocosity<<< numBlocks, numThreads >>>((float4*)dPos, (float4*)dVel, (float4*)dNewVel, dDens, c, cell);

	RX_CUERROR("xsphVisocosity kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}

/*!
 * �O���b�h��̖��x���Z�o
 * @param[out] dGridD �O���b�h��̖��x�l
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 * @param[in] nx,ny,nz �O���b�h��
 * @param[in] x0,y0,z0 �O���b�h�ŏ����W
 * @param[in] dx,dy,dz �O���b�h��
 */
void CuPbfGridDensity(float *dGridD, rxParticleCell cell, 
					  int nx, int ny, int nz, float x0, float y0, float z0, float dx, float dy, float dz)
{
	uint3  gnum = make_uint3(nx, ny, nz);
	float3 gmin = make_float3(x0, y0, z0);
	float3 glen = make_float3(dx, dy, dz);

	int numcell = gnum.x*gnum.y*gnum.z;

	int threads = 128;
	dim3 grid((numcell+threads-1)/threads, 1, 1);
	if(grid.x > 65535){
		grid.y = (grid.x+32768-1)/32768;
		grid.x = 32768;
	}

	// �J�[�l�����s
	pbfCalDensityInGrid<<<grid, threads>>>(dGridD, cell, gnum, gmin, glen);

	RX_CUERROR("pbfCalDensityInGrid Kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}

/*!
 * �p�[�e�B�N���@���̌v�Z
 * @param[out] dNrms �p�[�e�B�N���@��
 * @param[int] dDens �p�[�e�B�N�����x
 * @param[in]  cell �p�[�e�B�N���O���b�h�f�[�^
 */
void CuPbfNormal(float* dNrms, float* dDens, rxParticleCell cell)
{
	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(cell.uNumParticles, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	pbfCalNormal<<< numBlocks, numThreads >>>((float4*)dNrms, dDens, cell);

	RX_CUERROR("pbfCalNormal kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�

}










}   // extern "C"
