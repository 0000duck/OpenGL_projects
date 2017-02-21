/*! 
  @file rx_sph_kernel.cu
	
  @brief CUDA�ɂ��SPH
 
  @author Makoto Fujisawa
  @date 2009-08, 2011-06
*/
// FILE --rx_sph_kernel.cu--

#ifndef _RX_CUSPH_KERNEL_CU_
#define _RX_CUSPH_KERNEL_CU_


//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
#include "rx_cu_common.cu"

//-----------------------------------------------------------------------------
// �ϐ�
//-----------------------------------------------------------------------------
#if USE_TEX
texture<float4, cudaTextureType1D, cudaReadModeElementType> dSortedPosTex;
texture<float4, cudaTextureType1D, cudaReadModeElementType> dSortedVelTex;
texture<uint,   cudaTextureType1D, cudaReadModeElementType> dCellStartTex;
texture<uint,   cudaTextureType1D, cudaReadModeElementType> dCellEndTex;
#endif

// �V�~�����[�V�����p�����[�^(�R���X�^���g������)
__constant__ rxSimParams params;

__constant__ int RXNA[] = {0, -1, 1, -2, 2, -3, 3, -4, 4, -5, 5, -6, 6, -7, 7, -8, 8};






//-----------------------------------------------------------------------------
// �O���b�h
//-----------------------------------------------------------------------------
/*!
 * �O���b�h�ʒu�v�Z
 * @param[in] p ���W
 * @return �O���b�h���W
 */
__device__ 
int3 calcGridPos(float3 p)
{
	int3 gridPos;
	gridPos.x = floor((p.x-params.WorldOrigin.x)/params.CellWidth.x);
	gridPos.y = floor((p.y-params.WorldOrigin.y)/params.CellWidth.y);
	gridPos.z = floor((p.z-params.WorldOrigin.z)/params.CellWidth.z);

	gridPos.x = min(max(gridPos.x, 0), params.GridSize.x-1);
	gridPos.y = min(max(gridPos.y, 0), params.GridSize.y-1);
	gridPos.z = min(max(gridPos.z, 0), params.GridSize.z-1);

	return gridPos;
}

/*!
 * �O���b�h���W����1�����z�񒆂ł̈ʒu���v�Z
 * @param[in] gridPos �O���b�h���W
 * @return �A�h���X
 */
__device__ 
uint calcGridHash(int3 gridPos)
{
	return __umul24(__umul24(gridPos.z, params.GridSize.y), params.GridSize.x)+__umul24(gridPos.y, params.GridSize.x)+gridPos.x;
}



//-----------------------------------------------------------------------------
// �n�b�V��
//-----------------------------------------------------------------------------
/*!
 * �e�p�[�e�B�N���̃O���b�h�n�b�V���l
 * @param[out] gridParticleHash
 * @param[out] dSortedIndex
 * @param[in] pos �p�[�e�B�N���ʒu���i�[�����z��
 * @param[in] nprts �p�[�e�B�N����
 */
__global__
void calcHashD(uint*   dGridParticleHash, 
			   uint*   dSortedIndex, 
			   float4* dPos, 
			   uint	   nprts)
{
	uint index = __umul24(blockIdx.x, blockDim.x)+threadIdx.x;
	if(index >= nprts) return;
	
	volatile float4 p = dPos[index];

	int3 gridPos = calcGridPos(make_float3(p.x, p.y, p.z));
	uint hash = calcGridHash(gridPos);

	dGridParticleHash[index] = hash;
	dSortedIndex[index] = index;
}

/*!
 * �p�[�e�B�N���f�[�^���\�[�g���āC�n�b�V�����̊e�Z���̍ŏ��̃A�h���X������
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 * @param[in] oldPos �p�[�e�B�N���ʒu
 * @param[in] oldVel �p�[�e�B�N�����x
 */
__global__
void reorderDataAndFindCellStartD(rxParticleCell cell, float4* dSortedPos, float4* dSortedVel)
{
	extern __shared__ uint sharedHash[];	// �T�C�Y : blockSize+1
	uint index = __umul24(blockIdx.x,blockDim.x)+threadIdx.x;
	
	uint hash;
	if(index < cell.uNumParticles){
		hash = cell.dGridParticleHash[index];	// �n�b�V���l

		sharedHash[threadIdx.x+1] = hash;	// �n�b�V���l���V�F�A�[�h�������Ɋi�[

		if(index > 0 && threadIdx.x == 0){
			// �e�V�F�A�[�h�������̍ŏ��ׂ͗̃O���b�h�̃p�[�e�B�N���̃n�b�V���l���i�[
			sharedHash[0] = cell.dGridParticleHash[index-1];
		}
	}

	__syncthreads();
	
	if(index < cell.uNumParticles){
		// �C���f�b�N�X0�ł���C�������́C��O�̃p�[�e�B�N���̃O���b�h�n�b�V���l���قȂ�ꍇ�C
		// �p�[�e�B�N���͕����̈�̍ŏ�
		if(index == 0 || hash != sharedHash[threadIdx.x]){
			cell.dCellStart[hash] = index;
			if(index > 0){
				// ��O�̃p�[�e�B�N���́C��O�̕����̈�̍Ō�
				cell.dCellEnd[sharedHash[threadIdx.x]] = index;
			}
		}

		// �C���f�b�N�X���Ō�Ȃ�΁C�����̈�̍Ō�
		if(index == cell.uNumParticles-1){
			cell.dCellEnd[hash] = index+1;
		}

		// �ʒu�Ƒ��x�̃f�[�^����ёւ�
		// �\�[�g�����C���f�b�N�X�ŎQ�Ƃ��\�����T�����̃O���[�o���������A�N�Z�X���ɗ͗}���邽�߂Ƀf�[�^���̂��̂���ёւ���
		uint sortedIndex = cell.dSortedIndex[index];
		float4 pos = FETCH(dSortedPos, sortedIndex);
		float4 vel = FETCH(dSortedVel, sortedIndex);

		cell.dSortedPos[index] = pos;
		cell.dSortedVel[index] = vel;
	}
}




//-----------------------------------------------------------------------------
// MARK:SPH
//-----------------------------------------------------------------------------
/*!
 * �^����ꂽ�Z�����̃p�[�e�B�N���Ƃ̋������疧�x���v�Z
 * @param[in] gridPos �O���b�h�ʒu
 * @param[in] index �p�[�e�B�N���C���f�b�N�X
 * @param[in] pos �v�Z���W
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 * @return �Z�����̃p�[�e�B�N������v�Z�������x�l
 */
__device__
float calDensityCell(int3 gridPos, uint i, float3 pos0, rxParticleCell cell)
{
	uint gridHash = calcGridHash(gridPos);

	// �Z�����̃p�[�e�B�N���̃X�^�[�g�C���f�b�N�X
	uint startIndex = FETCHC(dCellStart, gridHash);

	float h = params.EffectiveRadius;
	float dens = 0.0f;
	if(startIndex != 0xffffffff){	// �Z������łȂ����̃`�F�b�N
		// �Z�����̃p�[�e�B�N���Ŕ���
		uint endIndex = FETCHC(dCellEnd, gridHash);
		for(uint j = startIndex; j < endIndex; ++j){
			//if(j == i) continue;

			float3 pos1 = make_float3(FETCHC(dSortedPos, j));

			float3 rij = pos0-pos1;
			float r = length(rij);

			if(r <= h){
				float q = h*h-r*r;
				dens += params.Mass*params.Wpoly6*q*q*q;
			}
		}
	}

	return dens;
}

/*!
 * �p�[�e�B�N�����x�v�Z(�J�[�l���֐�)
 * @param[out] newDens �p�[�e�B�N�����x
 * @param[out] newPres �p�[�e�B�N������
 * @param[in]  cell �p�[�e�B�N���O���b�h�f�[�^
 */
__global__
void sphCalDensity(float* newDens, float* newPres, rxParticleCell cell)
{
	// �p�[�e�B�N���C���f�b�N�X
	uint index = __mul24(blockIdx.x,blockDim.x)+threadIdx.x;
	if(index >= cell.uNumParticles) return;	
	
	float3 pos = make_float3(FETCHC(dSortedPos, index));	// �p�[�e�B�N���ʒu
	//int3 grid_pos = calcGridPos(pos);	// �p�[�e�B�N����������O���b�h�ʒu
	float h = params.EffectiveRadius;

	// �p�[�e�B�N�����͂̃O���b�h
	int3 grid_pos0, grid_pos1;
	grid_pos0 = calcGridPos(pos-make_float3(h));
	grid_pos1 = calcGridPos(pos+make_float3(h));

	// ���͂̃O���b�h���܂߂ċߖT�T���C���x�v�Z
	float dens = 0.0f;
	for(int z = grid_pos0.z; z <= grid_pos1.z; ++z){
		for(int y = grid_pos0.y; y <= grid_pos1.y; ++y){
			for(int x = grid_pos0.x; x <= grid_pos1.x; ++x){
				int3 n_grid_pos = make_int3(x, y, z);
				dens += calDensityCell(n_grid_pos, index, pos, cell);
			}
		}
	}

	// �K�X�萔���g�������͎Z�o
	float pres;
	pres = params.GasStiffness*(dens-params.Density);

	// ���x�ƈ��͒l�����ʂɏ�������
	uint oIdx = cell.dSortedIndex[index];
	newDens[oIdx] = dens;
	newPres[oIdx] = pres;
}




/*!
 * �^����ꂽ�Z�����̃p�[�e�B�N���Ƃ̋�������@�����v�Z
 * @param[in] gridPos �O���b�h�ʒu
 * @param[in] i �p�[�e�B�N���C���f�b�N�X
 * @param[in] pos �v�Z���W
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 * @return �Z�����̃p�[�e�B�N������v�Z�������x�l
 */
__device__
float3 calNormalCell(int3 gridPos, uint i, float3 pos0, float* dens, rxParticleCell cell)
{
	uint gridHash = calcGridHash(gridPos);

	// �Z�����̃p�[�e�B�N���̃X�^�[�g�C���f�b�N�X
	uint startIndex = FETCHC(dCellStart, gridHash);

	float h = params.EffectiveRadius;
	float3 nrm = make_float3(0.0f);
	if(startIndex != 0xffffffff){	// �Z������łȂ����̃`�F�b�N
		// �Z�����̃p�[�e�B�N���Ŕ���
		uint endIndex = FETCHC(dCellEnd, gridHash);

		for(uint j = startIndex; j < endIndex; ++j){
			if(j != i){
				float3 pos1 = make_float3(FETCHC(dSortedPos, j));

				float3 rij = pos0-pos1;
				float r = length(rij);

				if(r <= h && r > 0.0001){
					float d1 = dens[cell.dSortedIndex[j]];
					float q = h*h-r*r;

					nrm += (params.Mass/d1)*params.GWpoly6*q*q*rij;
				}

			}
		}
	}

	return nrm;
}


/*!
 * �p�[�e�B�N���@���v�Z(�J�[�l���֐�)
 * @param[out] newNrms �p�[�e�B�N���@��
 * @param[in] dens �p�[�e�B�N�����x
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 */
__global__
void sphCalNormal(float4* newNrms, float* dens, rxParticleCell cell)
{
	uint index = __mul24(blockIdx.x,blockDim.x)+threadIdx.x;
	if(index >= cell.uNumParticles) return;	
	
	float3 pos = make_float3(FETCHC(dSortedPos, index));	// �p�[�e�B�N���ʒu
	float h = params.EffectiveRadius;
	//int3 grid_pos = calcGridPos(pos);	// �p�[�e�B�N����������O���b�h�ʒu

	// �p�[�e�B�N�����͂̃O���b�h
	int3 grid_pos0, grid_pos1;
	grid_pos0 = calcGridPos(pos-make_float3(h));
	grid_pos1 = calcGridPos(pos+make_float3(h));

	// ���͂̃O���b�h���܂߂ċߖT�T���C���x�v�Z
	float3 nrm = make_float3(0.0f);
	for(int z = grid_pos0.z; z <= grid_pos1.z; ++z){
		for(int y = grid_pos0.y; y <= grid_pos1.y; ++y){
			for(int x = grid_pos0.x; x <= grid_pos1.x; ++x){
				int3 n_grid_pos = make_int3(x, y, z);
				nrm += calNormalCell(n_grid_pos, index, pos, dens, cell);
			}
		}
	}

	float l = length(nrm);
	if(l > 0){
		nrm /= l;
	}

	uint oIdx = cell.dSortedIndex[index];
	newNrms[oIdx] = make_float4(nrm, 0.0f);
}




/*!
 * �^����ꂽ�Z�����̃p�[�e�B�N���Ƃ̋�������͏���v�Z
 * @param[in] gridPos �O���b�h�ʒu
 * @param[in] i �p�[�e�B�N���C���f�b�N�X
 * @param[in] pos0 �v�Z���W
 * @param[in] vel0 �v�Z���W�̑��x
 * @param[in] dens0 �v�Z���W�̖��x
 * @param[in] pres0 �v�Z���W�̈���
 * @param[in] dens �p�[�e�B�N�����x
 * @param[in] pres �p�[�e�B�N������
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 * @return �Z�����̃p�[�e�B�N������v�Z�����͏�
 */
__device__
float3 calForceCell(int3 gridPos, uint i, float3 pos0, float3 vel0, float dens0, float pres0, 
					float* dens, float* pres, rxParticleCell cell)
{
	uint gridHash = calcGridHash(gridPos);

	// �Z�����̃p�[�e�B�N���̃X�^�[�g�C���f�b�N�X
	uint startIndex = FETCHC(dCellStart, gridHash);

	float h = params.EffectiveRadius;

	float3 frc = make_float3(0.0f);
	if(startIndex != 0xffffffff){	// �Z������łȂ����̃`�F�b�N
		// �Z�����̃p�[�e�B�N���Ŕ���
		uint endIndex = FETCHC(dCellEnd, gridHash);
		float prsi = pres0/(dens0*dens0);
		for(uint j = startIndex; j < endIndex; ++j){
			if(j != i){
				// �ߖT�p�[�e�B�N���̃p�����[�^
				float3 pos1 = make_float3(FETCHC(dSortedPos, j));
				float3 vel1 = make_float3(FETCHC(dSortedVel, j));

				float3 rij = pos0-pos1;
				float r = length(rij);

				if(r <= h && r > 0.0001){
					//float3 vel1 = make_float3(vel[cell.dSortedIndex[j]]);
					float dens1 = dens[cell.dSortedIndex[j]];
					float pres1 = pres[cell.dSortedIndex[j]];

					float3 vji = vel1-vel0;

					float prsj = pres1/(dens1*dens1);
					float q = h-r;

					// ���͍�
					frc += -dens0*params.Mass*(prsi+prsj)*params.GWspiky*q*q*rij/r;

					// �S����
					frc += params.Viscosity*params.Mass*(vji/dens1)*params.LWvisc*q;
				}
			}
		}
	}

	return frc;
}

/*!
 * �p�[�e�B�N���ɂ�����͂̌v�Z(�J�[�l���֐�)
 * @param[in] dens �p�[�e�B�N�����x
 * @param[in] pres �p�[�e�B�N������
 * @param[out] outFrc �p�[�e�B�N���ɂ������
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 */
__global__
void sphCalForces(float* dens, float* pres, float4* outFrc, rxParticleCell cell)
{
	uint index = __mul24(blockIdx.x,blockDim.x)+threadIdx.x;
	if(index >= cell.uNumParticles) return;	
	
	// �\�[�g�ςݔz�񂩂�p�[�e�B�N���f�[�^���擾
	float3 pos0 = make_float3(FETCHC(dSortedPos, index));
	float3 vel0 = make_float3(FETCHC(dSortedVel, index));

	int3 gridPos0 = calcGridPos(pos0);

	// �p�[�e�B�N���̃\�[�g�Ȃ��z���ł̃C���f�b�N�X
	uint oIdx = cell.dSortedIndex[index];

	float dens0 = dens[oIdx];
	float pres0 = pres[oIdx];

	float h = params.EffectiveRadius;
	int3 grid_pos0, grid_pos1;
	grid_pos0 = calcGridPos(pos0-make_float3(h));
	grid_pos1 = calcGridPos(pos0+make_float3(h));

	// ���͂̃O���b�h���܂߂ċߖT�T���C���͍��C�S�������v�Z
	float3 frc = make_float3(0.0f);
	for(int z = grid_pos0.z; z <= grid_pos1.z; ++z){
		for(int y = grid_pos0.y; y <= grid_pos1.y; ++y){
			for(int x = grid_pos0.x; x <= grid_pos1.x; ++x){
				int3 n_grid_pos = make_int3(x, y, z);

				frc += calForceCell(n_grid_pos, index, pos0, vel0, dens0, pres0, dens, pres, cell);
			}
		}
	}

	// �O��(�d�͂╂�͂Ȃ�)
	frc += params.Gravity*dens0;

	outFrc[oIdx] = make_float4(frc, 0.0f);
}


__device__
void calCollisionSolid(float3 &pos, float3 &vel, float dt)
{
	float d;
	float3 n;
	float3 cp;

	// �{�b�N�X�`��̃I�u�W�F�N�g�Ƃ̏Փ�
#if MAX_BOX_NUM
	for(int i = 0; i < params.BoxNum; ++i){
		if(params.BoxFlg[i] == 0) continue;
		
		collisionPointBox(pos, params.BoxCen[i], params.BoxExt[i], params.BoxRot[i], params.BoxInvRot[i], cp, d, n);

		if(d < 0.0){
			float res = params.Restitution;
			res = (res > 0) ? (res*fabs(d)/(dt*length(vel))) : 0.0f;
			vel -= (1+res)*n*dot(n, vel);
			pos = cp;
		}
	}
#endif

	// ���`��̃I�u�W�F�N�g�Ƃ̏Փ�
#if MAX_SPHERE_NUM
	for(int i = 0; i < params.SphereNum; ++i){
		if(params.SphereFlg[i] == 0) continue;

		collisionPointSphere(pos, params.SphereCen[i], params.SphereRad[i], cp, d, n);

		if(d < 0.0){
			float res = params.Restitution;
			res = (res > 0) ? (res*fabs(d)/(dt*length(vel))) : 0.0f;
			vel -= (1+res)*n*dot(n, vel);
			pos = cp;
		}
	}
#endif

	// ���͂̋��E�Ƃ̏Փ˔���
	float3 l0 = params.BoundaryMin;
	float3 l1 = params.BoundaryMax;
	collisionPointAABB(pos, 0.5*(l1+l0), 0.5*(l1-l0), cp, d, n);

	if(d < 0.0){
		float res = params.Restitution;
		res = (res > 0) ? (res*fabs(d)/(dt*length(vel))) : 0.0f;
		vel -= (1+res)*n*dot(n, vel);
		pos = cp;
	}
}

__device__
inline bool calCollisionPolygon(float3 &pos0, float3 &pos1, float3 &vel, float3 v0, float3 v1, float3 v2, float dt)
{
	float3 cp, n;
	if(intersectSegmentTriangle(pos0, pos1, v0, v1, v2, cp, n, params.ParticleRadius) == 1){
		float d = length(pos1-cp);
		n = normalize(n);

		float res = params.Restitution;
		res = (res > 0) ? (res*fabs(d)/(dt*length(vel))) : 0.0f;
		float3 vr = -(1+res)*n*dot(n, vel);

		float l = length(pos1-pos0);
		pos1 = cp+vr*(dt*d/l);
		vel += vr;

		return true;
	}
	return false;
}



/*!
 * �p�[�e�B�N���ʒu�C���x�̍X�V(Leap-Frog)
 * @param[inout] ppos �p�[�e�B�N���ʒu
 * @param[inout] pvel �p�[�e�B�N�����x
 * @param[in] pfrc �p�[�e�B�N���ɂ������
 * @param[in] dens �p�[�e�B�N�����x
 * @param[in] dt ���ԃX�e�b�v��
 * @param[in] nprts �p�[�e�B�N����
 */
__global__
void sphIntegrate(float4* ppos,	float4* pvel, 
				  float4* pfrc, float* dens, float dt, uint nprts)
{
	uint index = __umul24(blockIdx.x,blockDim.x)+threadIdx.x;
	if(index >= nprts) return;

	float3 x = make_float3(ppos[index]);
	float3 v = make_float3(pvel[index]);
	float3 f = make_float3(pfrc[index]);
	//float3 v_old = v;

	float dens0 = dens[index];

	// �X�V�ʒu�C���x�̍X�V
	v += dt*f/dens0;
	x += dt*v;

	// �ő́E���E�Ƃ̏Փ�
	calCollisionSolid(x, v, dt);

	// �ʒu�Ƒ��x�̍X�V
	ppos[index] = make_float4(x);
	pvel[index] = make_float4(v);
}



/*!
 * �p�[�e�B�N���ʒu�C���x�̍X�V(Leap-Frog)
 * @param[inout] ppos �p�[�e�B�N���ʒu
 * @param[inout] pvel �p�[�e�B�N�����x
 * @param[in] pfrc �p�[�e�B�N���ɂ������
 * @param[in] dens �p�[�e�B�N�����x
 * @param[in] vrts
 * @param[in] dt ���ԃX�e�b�v��
 * @param[in] nprts �p�[�e�B�N����
 */
__global__
void sphIntegrateWithPolygon(float4* ppos, float4* pvel, float4* pfrc, float* dens, 
							 float3* vrts, int3* tris, int tri_num, float dt, rxParticleCell cell)
{
	uint index = __umul24(blockIdx.x,blockDim.x)+threadIdx.x;
	if(index >= cell.uNumParticles) return;

	float3 x = make_float3(ppos[index]);
	float3 v = make_float3(pvel[index]);
	float3 f = make_float3(pfrc[index]);
	//float3 v_old = v;
	float3 x_old = x;

	float dens0 = dens[index];

	// �X�V�ʒu�C���x�̍X�V
	v += dt*f/dens0;
	x += dt*v;

	// �|���S���I�u�W�F�N�g�Ƃ̏Փ�
	int3 gridPos[2];
	gridPos[0] = calcGridPos(x_old);	// �ʒu�X�V�O�̃p�[�e�B�N����������O���b�h
	gridPos[1] = calcGridPos(x);		// �ʒu�X�V��̃p�[�e�B�N����������O���b�h
	for(int i = 0; i < 2; ++i){
		uint grid_hash = calcGridHash(gridPos[i]);
		uint start_index = cell.dPolyCellStart[grid_hash];
		if(start_index != 0xffffffff){	// �Z������łȂ����̃`�F�b�N

			uint end_index = cell.dPolyCellEnd[grid_hash];
			for(uint j = start_index; j < end_index; ++j){
				uint pidx = cell.dSortedPolyIdx[j];

				int3 idx = tris[pidx];
				if(calCollisionPolygon(x_old, x, v, vrts[idx.x], vrts[idx.y], vrts[idx.z], dt)){
				}
			}
		}
	}

	// �ő́E���E�Ƃ̏Փ�
	calCollisionSolid(x, v, dt);

	// �ʒu�Ƒ��x�̍X�V
	ppos[index] = make_float4(x);
	pvel[index] = make_float4(v);
}




/*!
 * �^����ꂽ�Z�����̃p�[�e�B�N���Ƃ̋������疧�x���v�Z
 * @param[in] gridPos �O���b�h�ʒu
 * @param[in] pos �v�Z���W
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 * @return �Z�����̃p�[�e�B�N������v�Z�������x�l
 */
__device__
float calDensityCellG(int3 gridPos, float3 pos0, rxParticleCell cell)
{
	uint gridHash = calcGridHash(gridPos);

	// �Z�����̃p�[�e�B�N���̃X�^�[�g�C���f�b�N�X
	uint startIndex = FETCHC(dCellStart, gridHash);

	float h = params.EffectiveRadius;
	float d = 0.0f;
	if(startIndex != 0xffffffff){	// �Z������łȂ����̃`�F�b�N
		// �Z�����̃p�[�e�B�N���Ŕ���
		uint endIndex = FETCHC(dCellEnd, gridHash);

		for(uint j = startIndex; j < endIndex; ++j){
			//if(j != index){
				float3 pos1 = make_float3(FETCHC(dSortedPos, j));

				float3 rij = pos0-pos1;
				float r = length(rij);

				if(r <= h){
					float q = h*h-r*r;

					d += params.Mass*params.Wpoly6*q*q*q;
				}

			//}
		}
	}

	return d;
}

/*!
 * �O���b�h��ł̖��x���v�Z
 * @param[out] GridD �O���b�h���x
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 * @param[in] gnum �O���b�h��
 * @param[in] gmin �O���b�h�ŏ����W
 * @param[in] glen �O���b�h��
 */
__global__
void sphCalDensityInGrid(float* GridD, rxParticleCell cell, 
					uint3 gnum, float3 gmin, float3 glen)
{
	uint blockId = __mul24(blockIdx.y, gridDim.x)+blockIdx.x;
	uint i = __mul24(blockId, blockDim.x)+threadIdx.x;

	uint3 gridPos = calcGridPosU(i, gnum);

	if(gridPos.x < gnum.x && gridPos.y < gnum.y && gridPos.z < gnum.z){
		float3 gpos;
		gpos.x = gmin.x+(gridPos.x)*glen.x;
		gpos.y = gmin.y+(gridPos.y)*glen.y;
		gpos.z = gmin.z+(gridPos.z)*glen.z;

		float d = 0.0f;

		int3 pgpos = calcGridPos(gpos);

		float h = params.EffectiveRadius;
		int3 grid_pos0, grid_pos1;
		grid_pos0 = calcGridPos(gpos-make_float3(h));
		grid_pos1 = calcGridPos(gpos+make_float3(h));

		for(int z = grid_pos0.z; z <= grid_pos1.z; ++z){
			for(int y = grid_pos0.y; y <= grid_pos1.y; ++y){
				for(int x = grid_pos0.x; x <= grid_pos1.x; ++x){
					int3 neighbourPos = make_int3(x, y, z);

					d += calDensityCellG(neighbourPos, gpos, cell);
				}
			}
		}

		GridD[gridPos.x+gridPos.y*gnum.x+gridPos.z*gnum.x*gnum.y] = d;
	}

}



//-----------------------------------------------------------------------------
// MARK:Anisotropic Kernel
//-----------------------------------------------------------------------------
/*!
 * �^����ꂽ�Z�����̃p�[�e�B�N���Ƃ̋�������d�ݕt�����ς��v�Z
 * @param[in] gridPos �O���b�h�ʒu
 * @param[in] i �p�[�e�B�N���C���f�b�N�X
 * @param[in] pos0 �v�Z���W
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 * @param[in] h �T�����a
 * @return �Z�����̃p�[�e�B�N������v�Z�����d�ݕt�����ϒl
 */
__device__
float4 calWeighedAvgPositionCell(int3 gridPos, uint i, float3 pos0, float h, rxParticleCell cell)
{
	uint gridHash = calcGridHash(gridPos);

	// �Z�����̃p�[�e�B�N���̃X�^�[�g�C���f�b�N�X
	uint startIndex = FETCHC(dCellStart, gridHash);

	float4 posw = make_float4(0.0f);	
	if(startIndex != 0xffffffff){	// �Z������łȂ����̃`�F�b�N
		// �Z�����̃p�[�e�B�N���Ŕ���
		uint endIndex = FETCHC(dCellEnd, gridHash);
		for(uint j = startIndex; j < endIndex; ++j){
			//if(j == i) continue;

			float3 pos1 = make_float3(FETCHC(dSortedPos, j));

			float3 rij = pos0-pos1;
			float r = length(rij);

			if(r < h){
				float q = (1.0f-r/h);
				float wij = q*q*q;
				posw += make_float4(pos1*wij, wij);
			}
		}
	}

	return posw;
}

/*!
 * �J�[�l�����S�ʒu�̕������Əd�ݕt�����ς̌v�Z(�J�[�l���֐�)
 * @param[out] upPos �������J�[�l�����S
 * @param[out] wPos �d�ݕt�����σp�[�e�B�N�����W 
 * @param[in]  lambda �������̂��߂̒萔
 * @param[in]  h �T�����a
 * @param[in]  cell �p�[�e�B�N���O���b�h�f�[�^
 */
__global__
void sphCalUpdatedPosition(float4* upPos, float4* wPos, float lambda, float h, rxParticleCell cell)
{
	// �p�[�e�B�N���C���f�b�N�X
	uint index = __mul24(blockIdx.x,blockDim.x)+threadIdx.x;
	if(index >= cell.uNumParticles) return;	
	
	float3 pos = make_float3(FETCHC(dSortedPos, index));	// �p�[�e�B�N���ʒu

	// �p�[�e�B�N�����͂̃O���b�h
	int3 grid_pos0, grid_pos1;
	grid_pos0 = calcGridPos(pos-make_float3(h));
	grid_pos1 = calcGridPos(pos+make_float3(h));

	// ���͂̃O���b�h���܂߂ċߖT�T���C�d�ݕt�����ψʒu���v�Z
	float4 posw  = make_float4(0.0f);
	for(int z = grid_pos0.z; z <= grid_pos1.z; ++z){
		for(int y = grid_pos0.y; y <= grid_pos1.y; ++y){
			for(int x = grid_pos0.x; x <= grid_pos1.x; ++x){
				int3 n_grid_pos = make_int3(x, y, z);
				posw += calWeighedAvgPositionCell(n_grid_pos, index, pos, h, cell);
			}
		}
	}

	// ���ʂ���������
	uint oIdx = cell.dSortedIndex[index];
	posw.x /= posw.w;
	posw.y /= posw.w;
	posw.z /= posw.w;
	posw.w = 0.0f;

	wPos[oIdx] = posw;
	upPos[oIdx]  = make_float4((1.0-lambda)*pos+lambda*make_float3(posw));
}


/*!
 * �^����ꂽ�Z�����̃p�[�e�B�N���Ƃ̋�������Covariance Matrix���v�Z
 * @param[in] gridPos �O���b�h�ʒu
 * @param[in] i �p�[�e�B�N���C���f�b�N�X
 * @param[in] pos0 �v�Z���W
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 * @param[in] h �T�����a
 * @return �Z�����̃p�[�e�B�N������v�Z�����d��
 */
__device__
float2 calCovarianceMatrixCell(int3 gridPos, uint i, float3 pos0, float h, matrix3x3 &c, float4 xiw, rxParticleCell cell)
{
	uint gridHash = calcGridHash(gridPos);

	// �Z�����̃p�[�e�B�N���̃X�^�[�g�C���f�b�N�X
	uint startIndex = FETCHC(dCellStart, gridHash);

	float2 wn = make_float2(0.0f);
	if(startIndex != 0xffffffff){	// �Z������łȂ����̃`�F�b�N
		// �Z�����̃p�[�e�B�N���Ŕ���
		uint endIndex = FETCHC(dCellEnd, gridHash);
		for(uint j = startIndex; j < endIndex; ++j){
			//if(j == i) continue;

			float3 pos1 = make_float3(FETCHC(dSortedPos, j));

			float3 rij = pos0-pos1;
			float r = length(rij);

			if(r < h){
				float q = (1.0f-r/h);
				float wij = q*q*q;

				float3 dxj = pos1-make_float3(xiw);

				c.e[0].x += wij*dxj.x*dxj.x;
				c.e[0].y += wij*dxj.x*dxj.y;
				c.e[0].z += wij*dxj.x*dxj.z;
				c.e[1].x += wij*dxj.y*dxj.x;
				c.e[1].y += wij*dxj.y*dxj.y;
				c.e[1].z += wij*dxj.y*dxj.z;
				c.e[2].x += wij*dxj.z*dxj.x;
				c.e[2].y += wij*dxj.z*dxj.y;
				c.e[2].z += wij*dxj.z*dxj.z;
				wn.x += wij;
				wn.y += 1.0;
			}
		}
	}

	return wn;
}


/*!
 * Covariance Matrix�̌v�Z
 * @param[out] PosW �d�ݕt�����σp�[�e�B�N�����W 
 * @param[out] CMat Covariance Matrix
 * @param[in]  lambda �������̂��߂̒萔
 * @param[in]  h �T�����a
 * @param[in]  cell �p�[�e�B�N���O���b�h�f�[�^
 */
__global__
void sphCalCovarianceMatrix(float4* PosW, matrix3x3 *CMat, float h, rxParticleCell cell)
{
	// �p�[�e�B�N���C���f�b�N�X
	uint index = __mul24(blockIdx.x,blockDim.x)+threadIdx.x;
	if(index >= cell.uNumParticles) return;	
	
	float3 pos = make_float3(FETCHC(dSortedPos, index));	// �p�[�e�B�N���ʒu

	// �p�[�e�B�N�����͂̃O���b�h
	int3 grid_pos0, grid_pos1;
	grid_pos0 = calcGridPos(pos-make_float3(h));
	grid_pos1 = calcGridPos(pos+make_float3(h));

	// ���͂̃O���b�h���܂߂ċߖT�T���C�d�ݕt�����ψʒu���v�Z
	float4 xiw  = make_float4(0.0f);
	for(int z = grid_pos0.z; z <= grid_pos1.z; ++z){
		for(int y = grid_pos0.y; y <= grid_pos1.y; ++y){
			for(int x = grid_pos0.x; x <= grid_pos1.x; ++x){
				int3 n_grid_pos = make_int3(x, y, z);
				xiw += calWeighedAvgPositionCell(n_grid_pos, index, pos, h, cell);
			}
		}
	}

	xiw.x /= xiw.w;
	xiw.y /= xiw.w;
	xiw.z /= xiw.w;

	//__syncthreads();

	// �s��̗v�f�̏�����
	matrix3x3 c;
	for(int k = 0; k < 3; ++k){
		c.e[k].x = 0.0;
		c.e[k].y = 0.0;
		c.e[k].z = 0.0;
	}

	// ���͂̃O���b�h���܂߂ċߖT�T���CCovariance Matrix���v�Z
	float2 wn = make_float2(0.0f);
	for(int z = grid_pos0.z; z <= grid_pos1.z; ++z){
		for(int y = grid_pos0.y; y <= grid_pos1.y; ++y){
			for(int x = grid_pos0.x; x <= grid_pos1.x; ++x){
				int3 n_grid_pos = make_int3(x, y, z);
				wn += calCovarianceMatrixCell(n_grid_pos, index, pos, h, c, xiw, cell);
			}
		}
	}

	for(int k = 0; k < 3; ++k){
		c.e[k].x /= wn.x;
		c.e[k].y /= wn.x;
		c.e[k].z /= wn.x;
	}

	xiw.w = wn.y;

	// ���ʂ���������
	uint oIdx = cell.dSortedIndex[index];
	PosW[oIdx] = xiw;
	CMat[oIdx] = c;
}

__device__
inline float RxPythag(const float a, const float b)
{
	float absa = abs(a), absb = abs(b);
	return (absa > absb ? absa*(float)sqrt((double)(1.0+(absb/absa)*(absb/absa))) :
		   (absb == 0.0 ? 0.0 : absb*(float)sqrt((double)(1.0+(absa/absb)*(absa/absb)))));
}

//! �ŏ��l����(2�l)
__device__
inline float RXD_MIN(const float &a, const float &b){ return ((a < b) ? a : b); }

//! �ő�l����(2�l)
__device__
inline float RXD_MAX(const float &a, const float &b){ return ((a > b) ? a : b); }

//! a�̕�����b�̕����ɂ��킹��
__device__
inline float RXD_SIGN2(const float &a, const float &b){ return b >= 0 ? (a >= 0 ? a : -a) : (a >= 0 ? -a : a); }

__device__
int svdecomp3(float w[3], float u[9], float v[9], float eps)
{
	bool flag;
	int i, its, j, jj, k, l, nm;
	float anorm, c, f, g, h, s, scale, x, y, z;
	float rv1[3];
	g = scale = anorm = 0.0;
	for(i = 0; i < 3; ++i){
		l = i+2;
		rv1[i] = scale*g;
		g = s = scale = 0.0;
		for(k = i; k < 3; ++k) scale += abs(u[k*3+i]);
		if(scale != 0.0){
			for(k = i; k < 3; ++k){
				u[k*3+i] /= scale;
				s += u[k*3+i]*u[k*3+i];
			}
			f = u[i*3+i];
			g = -RXD_SIGN2(sqrt(s), f);
			h = f*g-s;
			u[i*3+i] = f-g;
			for(j = l-1; j < 3; ++j){
				for(s = 0.0, k = i; k < 3; ++k) s += u[k*3+i]*u[k*3+j];
				f = s/h;
				for(k = i; k < 3; ++k) u[k*3+j] += f*u[k*3+i];
			}
			for(k = i; k < 3; ++k) u[k*3+i] *= scale;
		}

		w[i] = scale*g;
		g = s = scale = 0.0;
		if(i+1 <= 3 && i+1 != 3){
			for(k = l-1; k < 3; ++k) scale += abs(u[i*3+k]);
			if(scale != 0.0){
				for(k = l-1; k < 3; ++k){
					u[i*3+k] /= scale;
					s += u[i*3+k]*u[i*3+k];
				}
				f = u[i*3+l-1];
				g = -RXD_SIGN2(sqrt(s), f);
				h = f*g-s;
				u[i*3+l-1] = f-g;
				for(k = l-1; k < 3; ++k) rv1[k] = u[i*3+k]/h;
				for(j = l-1; j < 3; ++j){
					for(s = 0.0,k = l-1; k < 3; ++k) s += u[j*3+k]*u[i*3+k];
					for(k = l-1; k < 3; ++k) u[j*3+k] += s*rv1[k];
				}
				for(k = l-1; k < 3; ++k) u[i*3+k] *= scale;
			}
		}
		anorm = RXD_MAX(anorm, (abs(w[i])+abs(rv1[i])));
	}
	for(i = 2; i >= 0; --i){
		if(i < 2){
			if(g != 0.0){
				for(j = l; j < 3; ++j){
					v[j*3+i] = (u[i*3+j]/u[i*3+l])/g;
				}
				for(j = l; j < 3; ++j){
					for(s = 0.0, k = l; k < 3; ++k) s += u[i*3+k]*v[k*3+j];
					for(k = l; k < 3; ++k) v[k*3+j] += s*v[k*3+i];
				}
			}
			for(j = l; j < 3; ++j) v[i*3+j] = v[j*3+i] = 0.0;
		}
		v[i*3+i] = 1.0;
		g = rv1[i];
		l = i;
	}
	for(i = 2; i >= 0; --i){
		l = i+1;
		g = w[i];
		for(j = l; j < 3; ++j) u[i*3+j] = 0.0;
		if(g != 0.0){
			g = 1.0/g;
			for(j = l; j < 3; ++j){
				for(s = 0.0, k = l; k < 3; ++k) s += u[k*3+i]*u[k*3+j];
				f = (s/u[i*3+i])*g;
				for(k = i; k < 3; ++k) u[k*3+j] += f*u[k*3+i];
			}
			for(j = i; j < 3; ++j) u[j*3+i] *= g;
		}
		else{
			for(j = i; j < 3; ++j) u[j*3+i] = 0.0;
		}
		++u[i*3+i];
	}
	for(k = 2; k >= 0; --k){
		for(its = 0; its < 30; ++its){
			flag = true;
			for(l = k; l >= 0; --l){
				nm = l-1;
				if(l == 0 || abs(rv1[l]) <= eps*anorm){
					flag = false;
					break;
				}
				if(abs(w[nm]) <= eps*anorm) break;
			}
			if(flag){
				c = 0.0;
				s = 1.0;
				for(i = l; i < k+1; ++i){
					f = s*rv1[i];
					rv1[i] = c*rv1[i];
					if(abs(f) <= eps*anorm) break;
					g = w[i];
					h = RxPythag(f, g);
					w[i] = h;
					h = 1.0/h;
					c = g*h;
					s = -f*h;
					for(j = 0; j < 3; ++j){
						y = u[j*3+nm];
						z = u[j*3+i];
						u[j*3+nm] = y*c+z*s;
						u[j*3+i] = z*c-y*s;
					}
				}
			}
			z = w[k];
			if(l == k){
				if(z < 0.0){
					w[k] = -z;
					for(j = 0; j < 3; ++j) v[j*3+k] = -v[j*3+k];
				}
				break;
			}
			if(its == 29){
				//printf("no convergence in 30 svdcmp iterations");
				return 0;
			}
			x = w[l];
			nm = k-1;
			y = w[nm];
			g = rv1[nm];
			h = rv1[k];
			f = ((y-z)*(y+z)+(g-h)*(g+h))/(2.0*h*y);
			g = RxPythag(f, 1.0f);
			f = ((x-z)*(x+z)+h*((y/(f+RXD_SIGN2(g, f)))-h))/x;
			c = s = 1.0;
			for(j = l; j <= nm; ++j){
				i = j+1;
				g = rv1[i];
				y = w[i];
				h = s*g;
				g = c*g;
				z = RxPythag(f, h);
				rv1[j] = z;
				c = f/z;
				s = h/z;
				f = x*c+g*s;
				g = g*c-x*s;
				h = y*s;
				y *= c;
				for(jj = 0; jj < 3; ++jj){
					x = v[jj*3+j];
					z = v[jj*3+i];
					v[jj*3+j] = x*c+z*s;
					v[jj*3+i] = z*c-x*s;
				}
				z = RxPythag(f, h);
				w[j] = z;
				if(z){
					z = 1.0/z;
					c = f*z;
					s = h*z;
				}
				f = c*g+s*y;
				x = c*y-s*g;
				for(jj = 0; jj < 3; ++jj){
					y = u[jj*3+j];
					z = u[jj*3+i];
					u[jj*3+j] = y*c+z*s;
					u[jj*3+i] = z*c-y*s;
				}
			}
			rv1[l] = 0.0;
			rv1[k] = f;
			w[k] = x;
		}
	}

	// reorder
	int inc = 1;
	float sw;
	float su[3], sv[3];

	do{
		inc *= 3;
		inc++; 
	}while(inc <= 3);

	do{
		inc /= 3;
		for(i = inc; i < 3; ++i){
			sw = w[i];
			for(k = 0; k < 3; ++k) su[k] = u[k*3+i];
			for(k = 0; k < 3; ++k) sv[k] = v[k*3+i];
			j = i;
			while (w[j-inc] < sw){
				w[j] = w[j-inc];
				for(k = 0; k < 3; ++k) u[k*3+j] = u[k*3+j-inc];
				for(k = 0; k < 3; ++k) v[k*3+j] = v[k*3+j-inc];
				j -= inc;
				if (j < inc) break;
			}
			w[j] = sw;
			for(k = 0; k < 3; ++k) u[k*3+j] = su[k];
			for(k = 0; k < 3; ++k) v[k*3+j] = sv[k];

		}
	}while(inc > 1);

	for(k = 0; k < 3; ++k){
		s = 0;
		for(i = 0; i < 3; ++i) if(u[i*3+k] < 0.) s++;
		for(j = 0; j < 3; ++j) if(v[j*3+k] < 0.) s++;
		if(s > 3){
			for(i = 0; i < 3; ++i) u[i*3+k] = -u[i*3+k];
			for(j = 0; j < 3; ++j) v[j*3+k] = -v[j*3+k];
		}
	}

	return 1;
}

/*!
 * ���ْl�����ɂ��ŗL�l���v�Z
 * @param[in]  CMat Covariance Matrix
 * @param[in]  posw �d�ݕt�����ψʒu
 * @param[out] eigen �ŗL�l
 * @param[out] RMat �ŗL�x�N�g��(��]�s��)
 * @param[in]  numParticles �p�[�e�B�N����
 */
__global__
void sphSVDecomposition(matrix3x3 *CMat, float4* posw, float3* eigen, matrix3x3 *RMat, uint numParticles)
{
	// �p�[�e�B�N���C���f�b�N�X
	uint index = __mul24(blockIdx.x,blockDim.x)+threadIdx.x;
	if(index >= numParticles) return;

	int n = (int)posw[index].w;
	matrix3x3 tmp = CMat[index];
	float u[9], v[9], w[3];
	for(int i = 0; i < 3; ++i){
		u[i*3+0] = tmp.e[i].x;
		u[i*3+1] = tmp.e[i].y;
		u[i*3+2] = tmp.e[i].z;
	}

	// ���ْl����
	svdecomp3(w, u, v, 1.0e-10);
	
	float3 sigma;
	sigma.x = w[0];
	sigma.y = w[1];
	sigma.z = w[2];
	for(int i = 0; i < 3; ++i){
		tmp.e[i].x = u[i*3+0];
		tmp.e[i].y = u[i*3+1];
		tmp.e[i].z = u[i*3+2];
	}
	
	int ne = 10;
	float ks = 1400;
	float kn = 0.5;
	float kr = 4.0;
	if(n > ne){
		float s0 = sigma.x/kr;
		sigma.y = (sigma.y >= s0 ? sigma.y : s0);
		sigma.z = (sigma.z >= s0 ? sigma.z : s0);
		sigma *= ks;
	}
	else{
		sigma = make_float3(kn*1.0f);
	}

	eigen[index] = sigma;
	RMat[index]  = tmp;
}

__device__
inline float calDeterminant(matrix3x3 &mat)
{
	float d = mat.e[0].x*mat.e[1].y*mat.e[2].z- 
			  mat.e[0].x*mat.e[2].y*mat.e[1].z+ 
			  mat.e[1].x*mat.e[2].y*mat.e[0].z- 
			  mat.e[1].x*mat.e[0].y*mat.e[2].z+ 
			  mat.e[2].x*mat.e[0].y*mat.e[1].z- 
			  mat.e[2].x*mat.e[1].y*mat.e[0].z;
	return d;
}

__device__
inline matrix3x3 calInverse(matrix3x3 &mat)
{
	matrix3x3 inv_mat;

	float d = mat.e[0].x*mat.e[1].y*mat.e[2].z- 
			  mat.e[0].x*mat.e[2].y*mat.e[1].z+ 
			  mat.e[1].x*mat.e[2].y*mat.e[0].z- 
			  mat.e[1].x*mat.e[0].y*mat.e[2].z+ 
			  mat.e[2].x*mat.e[0].y*mat.e[1].z- 
			  mat.e[2].x*mat.e[1].y*mat.e[0].z;

	if(d == 0) d = 1;

	inv_mat.e[0].x =  (mat.e[1].y*mat.e[2].z-mat.e[1].z*mat.e[2].y)/d;
	inv_mat.e[1].x = -(mat.e[0].y*mat.e[2].z-mat.e[0].z*mat.e[2].y)/d;
	inv_mat.e[2].x =  (mat.e[0].y*mat.e[1].z-mat.e[0].z*mat.e[1].y)/d;
	inv_mat.e[0].y = -(mat.e[1].x*mat.e[2].z-mat.e[1].z*mat.e[2].x)/d;
	inv_mat.e[1].y =  (mat.e[0].x*mat.e[2].z-mat.e[0].z*mat.e[2].x)/d;
	inv_mat.e[2].y = -(mat.e[0].x*mat.e[1].z-mat.e[0].z*mat.e[1].x)/d;
	inv_mat.e[0].z =  (mat.e[1].x*mat.e[2].y-mat.e[1].y*mat.e[2].x)/d;
	inv_mat.e[1].z = -(mat.e[0].x*mat.e[2].y-mat.e[0].y*mat.e[2].x)/d;
	inv_mat.e[2].z =  (mat.e[0].x*mat.e[1].y-mat.e[0].y*mat.e[1].x)/d;

	return inv_mat;
}

/*!
 * �ŗL�l�C�ŗL�x�N�g��(��]�s��)����ό`�s����v�Z
 * @param[in]  eigen �ŗL�l
 * @param[in]  RMat �ŗL�x�N�g��(��]�s��)
 * @param[out] GMat �ό`�s��
 * @param[in]  numParticles �p�[�e�B�N����
 */
__global__
void sphCalTransformMatrix(float3* eigen, matrix3x3 *RMat, matrix3x3 *GMat, uint numParticles)
{
	// �p�[�e�B�N���C���f�b�N�X
	uint index = __mul24(blockIdx.x,blockDim.x)+threadIdx.x;
	if(index >= numParticles) return;

	float3 sigma = eigen[index];
	matrix3x3 R = RMat[index];
	matrix3x3 G;

	for(int j = 0; j < 3; ++j){
		G.e[j].x = R.e[j].x*R.e[0].x/sigma.x+R.e[j].y*R.e[0].y/sigma.y+R.e[j].z*R.e[0].z/sigma.z;
		G.e[j].y = R.e[j].x*R.e[1].x/sigma.x+R.e[j].y*R.e[1].y/sigma.y+R.e[j].z*R.e[1].z/sigma.z;
		G.e[j].z = R.e[j].x*R.e[2].x/sigma.x+R.e[j].y*R.e[2].y/sigma.y+R.e[j].z*R.e[2].z/sigma.z;
	}

	float max_diag = -100000000.0f;
	if(G.e[0].x > max_diag) max_diag = G.e[0].x;
	if(G.e[1].y > max_diag) max_diag = G.e[1].y;
	if(G.e[2].z > max_diag) max_diag = G.e[2].z;

	for(int j = 0; j < 3; ++j){
		G.e[j].x /= max_diag;
		G.e[j].y /= max_diag;
		G.e[j].z /= max_diag;
	}

	//G = calInverse(G);

	GMat[index] = G;
}



/*!
 * �^����ꂽ�Z�����̃p�[�e�B�N���Ƃ̋������疧�x���v�Z
 * @param[in] gridPos �O���b�h�ʒu
 * @param[in] pos �v�Z���W
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 * @return �Z�����̃p�[�e�B�N������v�Z�������x�l
 */
__device__
float calDensityAnisoCellG(int3 gridPos, float3 pos0, float h, matrix3x3 *GMat, rxParticleCell cell)
{
	uint gridHash = calcGridHash(gridPos);

	// �Z�����̃p�[�e�B�N���̃X�^�[�g�C���f�b�N�X
	uint startIndex = FETCHC(dCellStart, gridHash);

	float d = 0.0f;
	if(startIndex != 0xffffffff){	// �Z������łȂ����̃`�F�b�N
		// �Z�����̃p�[�e�B�N���Ŕ���
		uint endIndex = FETCHC(dCellEnd, gridHash);

		for(uint j = startIndex; j < endIndex; ++j){
			//if(j != index){
				float3 pos1 = make_float3(FETCHC(dSortedPos, j));
				uint jdx = cell.dSortedIndex[j];

				float3 rij = pos0-pos1;
				matrix3x3 Gj = GMat[jdx];
				float3 rg;
				rg.x = Gj.e[0].x*rij.x+Gj.e[0].y*rij.y+Gj.e[0].z*rij.z;
				rg.y = Gj.e[1].x*rij.x+Gj.e[1].y*rij.y+Gj.e[1].z*rij.z;
				rg.z = Gj.e[2].x*rij.x+Gj.e[2].y*rij.y+Gj.e[2].z*rij.z;

				float r = length(rg);

				if(r <= h){
					float q = h*h-r*r;
					float detG = calDeterminant(Gj);

					d += detG*params.Mass*params.Wpoly6*q*q*q;
				}

			//}
		}
	}

	return d;
}

/*!
 * �O���b�h��ł̖��x���v�Z
 * @param[out] GridD �O���b�h���x
 * @param[in] cell �p�[�e�B�N���O���b�h�f�[�^
 * @param[in] gnum �O���b�h��
 * @param[in] gmin �O���b�h�ŏ����W
 * @param[in] glen �O���b�h��
 */
__global__
void sphCalDensityAnisoInGrid(float* GridD, matrix3x3 *GMat, float Emax, 
							  rxParticleCell cell, uint3 gnum, float3 gmin, float3 glen)
{
	uint blockId = __mul24(blockIdx.y, gridDim.x)+blockIdx.x;
	uint i = __mul24(blockId, blockDim.x)+threadIdx.x;

	uint3 gridPos = calcGridPosU(i, gnum);

	if(gridPos.x < gnum.x && gridPos.y < gnum.y && gridPos.z < gnum.z){
		float3 gpos;
		gpos.x = gmin.x+(gridPos.x)*glen.x;
		gpos.y = gmin.y+(gridPos.y)*glen.y;
		gpos.z = gmin.z+(gridPos.z)*glen.z;

		matrix3x3 G = GMat[i];
		float detG = calDeterminant(G);
		int3 pgpos = calcGridPos(gpos);

		float h = params.EffectiveRadius;
		int3 grid_pos0, grid_pos1;
		grid_pos0 = calcGridPos(gpos-make_float3(h*3));
		grid_pos1 = calcGridPos(gpos+make_float3(h*3));

		float P = 0.0f;
		for(int z = grid_pos0.z; z <= grid_pos1.z; ++z){
			for(int y = grid_pos0.y; y <= grid_pos1.y; ++y){
				for(int x = grid_pos0.x; x <= grid_pos1.x; ++x){
					int3 neighbourPos = make_int3(x, y, z);

					P += calDensityAnisoCellG(neighbourPos, gpos, h, GMat, cell);
				}
			}
		}

		GridD[gridPos.x+gridPos.y*gnum.x+gridPos.z*gnum.x*gnum.y] = P;
	}

}


#endif // #ifndef _RX_CUSPH_KERNEL_CU_



