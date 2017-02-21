/*! 
  @file rx_sph.cu
	
  @brief CUDA�ɂ��SPH

*/
// FILE --rx_sph.cu--


//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
#include <cstdio>
#include <GL/glew.h>

#include <GL/freeglut.h>

#include "rx_sph_kernel.cu"

//#include "rx_cu_funcs.cuh"
#include <thrust/device_vector.h>
#include <thrust/scan.h>



//-----------------------------------------------------------------------------
// MARK:�O���[�o���ϐ�
//-----------------------------------------------------------------------------
cudaArray *g_caNoiseTile = 0;
float *g_dNoiseTile[3] = {0, 0, 0};
uint g_udNoiseTileSize = 0;
uint g_uNoiseTileNum[3*3] = {0, 0, 0,  0, 0, 0,  0, 0, 0};


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


// �O���b�h���u���b�N���C�u���b�N���X���b�h���̌v�Z
void computeGridSize(uint n, uint blockSize, uint &numBlocks, uint &numThreads)
{
	numThreads = min(blockSize, n);
	numBlocks = DivCeil(n, numThreads);
}


//-----------------------------------------------------------------------------
// MARK:3D SPH
//-----------------------------------------------------------------------------
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
	
	RX_CUERROR("Kernel execution failed");	// �J�[�l���G���[�`�F�b�N
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

#if USE_TEX
	RX_CUCHECK(cudaBindTexture(0, dSortedPosTex, oldPos, cell.uNumParticles*sizeof(float4)));
	RX_CUCHECK(cudaBindTexture(0, dSortedVelTex, oldVel, cell.uNumParticles*sizeof(float4)));
#endif

	uint smemSize = sizeof(uint)*(numThreads+1);

	// �J�[�l�����s
	reorderDataAndFindCellStartD<<< numBlocks, numThreads, smemSize>>>(cell, (float4*)oldPos, (float4*)oldVel);

	RX_CUERROR("Kernel execution failed: CuReorderDataAndFindCellStartD");
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�

#if USE_TEX
	RX_CUCHECK(cudaUnbindTexture(dSortedPosTex));
	RX_CUCHECK(cudaUnbindTexture(dSortedVelTex));
#endif
}


/*!
 * �p�[�e�B�N�����x�̌v�Z(�J�[�l���Ăяo��)
 * @param[out] dDens �p�[�e�B�N�����x
 * @param[out] dPres �p�[�e�B�N������
 * @param[in]  cell �p�[�e�B�N���O���b�h�f�[�^
 */
void CuSphDensity(float* dDens, float* dPres, rxParticleCell cell)
{
	// MRK:CuSphDensity2D
#if USE_TEX
	RX_CUCHECK(cudaBindTexture(0, dSortedPosTex, cell.dSortedPos, cell.uNumParticles*sizeof(float4)));
	RX_CUCHECK(cudaBindTexture(0, dCellStartTex, cell.dCellStart, cell.uNumCells*sizeof(uint)));
	RX_CUCHECK(cudaBindTexture(0, dCellEndTex, cell.dCellEnd, cell.uNumCells*sizeof(uint)));	
#endif
	//RX_CUCHECK(cudaMemset((void*)dNewDens, 0, sizeof(float2)*cell.uNumParticles));

	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(cell.uNumParticles, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	sphCalDensity<<< numBlocks, numThreads >>>(dDens, dPres, cell);

	RX_CUERROR("sphCalDensity kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�

#if USE_TEX
	RX_CUCHECK(cudaUnbindTexture(dSortedPosTex));
	RX_CUCHECK(cudaUnbindTexture(dCellStartTex));
	RX_CUCHECK(cudaUnbindTexture(dCellEndTex));
#endif
}

/*!
 * �p�[�e�B�N���@���̌v�Z
 * @param[out] dNewDens �p�[�e�B�N�����x
 * @param[out] dNewPres �p�[�e�B�N������
 * @param[in]  cell �p�[�e�B�N���O���b�h�f�[�^
 */
void CuSphNormal(float* dNrms, float* dDens, rxParticleCell cell)
{
	// MRK:CuSphNormal

	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(cell.uNumParticles, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	sphCalNormal<<< numBlocks, numThreads >>>((float4*)dNrms, dDens, cell);

	RX_CUERROR("sphCalNormal kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
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
void CuSphForces(float* dDens, float* dPres, float* dFrc, rxParticleCell cell, float dt)
{
#if USE_TEX
	RX_CUCHECK(cudaBindTexture(0, dSortedPosTex, cell.dSortedPos, cell.uNumParticles*sizeof(float4)));
	RX_CUCHECK(cudaBindTexture(0, dSortedVelTex, cell.dSortedVel, cell.uNumParticles*sizeof(float4)));
	RX_CUCHECK(cudaBindTexture(0, dCellStartTex, cell.dCellStart, cell.uNumCells*sizeof(uint)));
	RX_CUCHECK(cudaBindTexture(0, dCellEndTex, cell.dCellEnd, cell.uNumCells*sizeof(uint)));	
#endif

	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(cell.uNumParticles, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	sphCalForces<<< numBlocks, numThreads >>>(dDens, dPres, (float4*)dFrc, cell);

	RX_CUERROR("calForcesSPH kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�

#if USE_TEX
	RX_CUCHECK(cudaUnbindTexture(dSortedPosTex));
	RX_CUCHECK(cudaUnbindTexture(dSortedVelTex));
	RX_CUCHECK(cudaUnbindTexture(dCellStartTex));
	RX_CUCHECK(cudaUnbindTexture(dCellEndTex));
#endif
}

/*!
 * �p�[�e�B�N���ʒu�C���x�̍X�V
 * @param[inout] pos �p�[�e�B�N���ʒu
 * @param[inout] vel �p�[�e�B�N�����x
 * @param[inout] velOld �O�X�e�b�v�̃p�[�e�B�N�����x
 * @param[in] frc �p�[�e�B�N���ɂ������
 * @param[in] dens �p�[�e�B�N�����x
 * @param[in] dt ���ԃX�e�b�v��
 * @param[in] nprts �p�[�e�B�N����
 */
void CuSphIntegrate(float* pos, float* vel, float* frc, float* dens, 
					float dt, uint nprts)
{
	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(nprts, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	sphIntegrate<<< numBlocks, numThreads >>>((float4*)pos, (float4*)vel, (float4*)frc, dens, 
											  dt, nprts);
	
	RX_CUERROR("sphIntegrate kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}


/*!
 * �p�[�e�B�N���ʒu�C���x�̍X�V
 * @param[inout] pos �p�[�e�B�N���ʒu
 * @param[inout] vel �p�[�e�B�N�����x
 * @param[inout] velOld �O�X�e�b�v�̃p�[�e�B�N�����x
 * @param[in] frc �p�[�e�B�N���ɂ������
 * @param[in] dens �p�[�e�B�N�����x
 * @param[in] dt ���ԃX�e�b�v��
 * @param[in] nprts �p�[�e�B�N����
 */
void CuSphIntegrateWithPolygon(float* pos, float* vel, float* frc, float* dens, 
							   float* vrts, int* tris, int tri_num, float dt, rxParticleCell cell)
{
	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(cell.uNumParticles, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	sphIntegrateWithPolygon<<< numBlocks, numThreads >>>((float4*)pos, (float4*)vel, (float4*)frc, dens, 
											   (float3*)vrts, (int3*)tris, tri_num, dt, cell);
	
	RX_CUERROR("sphIntegrate kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}

/*!
 * �O���b�h��̖��x���Z�o
 * @param[out] dGridD �O���b�h��̖��x�l
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 * @param[in] nx,ny �O���b�h��
 * @param[in] x0,y0 �O���b�h�ŏ����W
 * @param[in] dx,dy �O���b�h��
 */
void CuSphGridDensity(float *dGridD, rxParticleCell cell, 
					  int nx, int ny, int nz, float x0, float y0, float z0, float dx, float dy, float dz)
{
#if USE_TEX
	RX_CUCHECK(cudaBindTexture(0, dSortedPosTex, cell.dSortedPos, cell.uNumParticles*sizeof(float4)));
	RX_CUCHECK(cudaBindTexture(0, dCellStartTex, cell.dCellStart, cell.uNumCells*sizeof(uint)));
	RX_CUCHECK(cudaBindTexture(0, dCellEndTex, cell.dCellEnd, cell.uNumCells*sizeof(uint)));	
#endif

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
	sphCalDensityInGrid<<<grid, threads>>>(dGridD, cell, gnum, gmin, glen);

	RX_CUERROR("Kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�

#if USE_TEX
	RX_CUCHECK(cudaUnbindTexture(dSortedPosTex));
	RX_CUCHECK(cudaUnbindTexture(dCellStartTex));
	RX_CUCHECK(cudaUnbindTexture(dCellEndTex));
#endif
}


//-----------------------------------------------------------------------------
// MARK:Anisotropic Kernel
//-----------------------------------------------------------------------------
/*!
 * �J�[�l�����S�ʒu�̍X�V�Əd�ݕt�����ς̌v�Z(�J�[�l���֐�)
 * @param[out] dUpPos �X�V�J�[�l�����S
 * @param[out] dPosW �d�ݕt�����σp�[�e�B�N�����W 
 * @param[in]  lambda �������̂��߂̒萔
 * @param[in]  h �T�����a
 * @param[in]  cell �p�[�e�B�N���O���b�h�f�[�^
 */
void CuSphCalUpdatedPosition(float* dUpPos, float* dPosW, float lambda, float h, rxParticleCell cell)
{
	// MRK:CuSphCalUpdatedPosition
#if USE_TEX
	RX_CUCHECK(cudaBindTexture(0, dSortedPosTex, cell.dSortedPos, cell.uNumParticles*sizeof(float4)));
	RX_CUCHECK(cudaBindTexture(0, dCellStartTex, cell.dCellStart, cell.uNumCells*sizeof(uint)));
	RX_CUCHECK(cudaBindTexture(0, dCellEndTex, cell.dCellEnd, cell.uNumCells*sizeof(uint)));	
#endif

	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(cell.uNumParticles, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	sphCalUpdatedPosition<<< numBlocks, numThreads >>>((float4*)dUpPos, (float4*)dPosW, lambda, h, cell);

	RX_CUERROR("sphCalUpdatedPosition kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�

#if USE_TEX
	RX_CUCHECK(cudaUnbindTexture(dSortedPosTex));
	RX_CUCHECK(cudaUnbindTexture(dCellStartTex));
	RX_CUCHECK(cudaUnbindTexture(dCellEndTex));
#endif
}

/*!
 * �������ʒu�ł̏d�ݕt�����ψʒu�̍Čv�Z��covariance matrix�̌v�Z
 * @param[out] dPosW �d�ݕt�����σp�[�e�B�N�����W 
 * @param[out] dCMat Covariance Matrix
 * @param[in]  h �T�����a
 * @param[in]  cell �p�[�e�B�N���O���b�h�f�[�^
 */
void CuSphCalCovarianceMatrix(float* dPosW, float* dCMat, float h, rxParticleCell cell)
{
	// MRK:CuSphCalCovarianceMatrix
#if USE_TEX
	RX_CUCHECK(cudaBindTexture(0, dSortedPosTex, cell.dSortedPos, cell.uNumParticles*sizeof(float4)));
	RX_CUCHECK(cudaBindTexture(0, dCellStartTex, cell.dCellStart, cell.uNumCells*sizeof(uint)));
	RX_CUCHECK(cudaBindTexture(0, dCellEndTex, cell.dCellEnd, cell.uNumCells*sizeof(uint)));	
#endif

	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(cell.uNumParticles, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	sphCalCovarianceMatrix<<< numBlocks, numThreads >>>((float4*)dPosW, (matrix3x3*)dCMat, h, cell);

	RX_CUERROR("sphCalCovarianceMatrix kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�

#if USE_TEX
	RX_CUCHECK(cudaUnbindTexture(dSortedPosTex));
	RX_CUCHECK(cudaUnbindTexture(dCellStartTex));
	RX_CUCHECK(cudaUnbindTexture(dCellEndTex));
#endif
}

/*!
 * ���ْl�����ɂ��ŗL�l���v�Z
 * @param[in]  dC Covariance Matrix
 * @param[in]  dPosW �d�ݕt�����ψʒu
 * @param[out] dEigen �ŗL�l
 * @param[out] dR �ŗL�x�N�g��(��]�s��)
 * @param[in]  numParticles �p�[�e�B�N����
 */
void CuSphSVDecomposition(float* dC, float* dPosW, float* dEigen, float* dR, uint numParticles)
{
	// MRK:CuSphCalTransformMatrix

	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(numParticles, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	sphSVDecomposition<<< numBlocks, numThreads >>>((matrix3x3*)dC, (float4*)dPosW, (float3*)dEigen, (matrix3x3*)dR, numParticles);

	RX_CUERROR("sphCalTransformMatrix kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}

/*!
 * �ŗL�l�C�ŗL�x�N�g��(��]�s��)����ό`�s����v�Z
 * @param[in]  dEigen �ŗL�l
 * @param[in]  dR �ŗL�x�N�g��(��]�s��)
 * @param[out] dG �ό`�s��
 * @param[in]  numParticles �p�[�e�B�N����
 */
void CuSphCalTransformMatrix(float* dEigen, float* dR, float *dG, uint numParticles)
{
	// MRK:CuSphCalTransformMatrix

	// 1�X���b�h/�p�[�e�B�N��
	uint numThreads, numBlocks;
	computeGridSize(numParticles, THREAD_NUM, numBlocks, numThreads);

	// �J�[�l�����s
	sphCalTransformMatrix<<< numBlocks, numThreads >>>((float3*)dEigen, (matrix3x3*)dR, (matrix3x3*)dG, numParticles);

	RX_CUERROR("sphCalTransformMatrix kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�
}


/*!
 * �O���b�h��̖��x���Z�o
 * @param[out] dGridD �O���b�h��̖��x�l
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 * @param[in] nx,ny �O���b�h��
 * @param[in] x0,y0 �O���b�h�ŏ����W
 * @param[in] dx,dy �O���b�h��
 */
void CuSphGridDensityAniso(float *dGridD, float *dG, float Emax, rxParticleCell cell, 
						   int nx, int ny, int nz, float x0, float y0, float z0, float dx, float dy, float dz)
{
#if USE_TEX
	RX_CUCHECK(cudaBindTexture(0, dSortedPosTex, cell.dSortedPos, cell.uNumParticles*sizeof(float4)));
	RX_CUCHECK(cudaBindTexture(0, dCellStartTex, cell.dCellStart, cell.uNumCells*sizeof(uint)));
	RX_CUCHECK(cudaBindTexture(0, dCellEndTex, cell.dCellEnd, cell.uNumCells*sizeof(uint)));	
#endif

	uint3  gnum = make_uint3(nx, ny, nz);
	float3 gmin = make_float3(x0, y0, z0);
	float3 glen = make_float3(dx, dy, dz);

	int numcell = gnum.x*gnum.y*gnum.z;

	int threads = THREAD_NUM;
	dim3 grid((numcell+threads-1)/threads, 1, 1);
	if(grid.x > 65535){
		grid.y = (grid.x+32768-1)/32768;
		grid.x = 32768;
	}

	// �J�[�l�����s
	sphCalDensityAnisoInGrid<<<grid, threads>>>(dGridD, (matrix3x3*)dG, Emax, cell, gnum, gmin, glen);

	RX_CUERROR("Kernel execution failed");	// �J�[�l�����s�G���[�`�F�b�N
	RX_CUCHECK(cudaThreadSynchronize());		// �S�ẴX���b�h���I���̂�҂�

#if USE_TEX
	RX_CUCHECK(cudaUnbindTexture(dSortedPosTex));
	RX_CUCHECK(cudaUnbindTexture(dCellStartTex));
	RX_CUCHECK(cudaUnbindTexture(dCellEndTex));
#endif
}


}   // extern "C"
