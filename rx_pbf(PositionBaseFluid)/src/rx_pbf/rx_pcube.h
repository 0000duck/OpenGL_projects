#ifndef _RX_PCUBE_
#define _RX_PCUBE_

#include "rx_utility.h"

#include <vector>
using namespace std;


namespace RXFunc
{
	/*!
	 * ���ʂ�AABB�̌�������
	 *  - Real-Time Rendering Third Edition, p755
	 * @param[in] n,d	���� n�Ex+d=0
	 * @param[in] c,l   AABB�̒��S�ƕӂ̒����̔���
	 */
	inline bool plane_aabb(const Vec3 &n, const double &d, 
						   const Vec3 &c, const Vec3 &h)
	{
		double e = h[0]*fabs(n[0])+h[1]*fabs(n[1])+h[2]*fabs(n[2]);
		double s = dot(c, n)+d;
		if(s-e > 0) return false;	// ����ԊO(�@���̋t��)
		if(s+e < 0) return false;	// ����ԓ�(�@����)
		return true;
	}

	/*!
	 * �O�p�`��AABB�̌�������
	 *  - Real-Time Rendering Third Edition, p760
	 * @param[in] u0,u1,u2	�O�p�`�̒��_
	 * @param[in] n			�O�p�`�̖@��
	 * @param[in] c,l   �����̂̒��S�ƕӂ̒����̔���
	 * @return ������true
	 */
	inline bool triangle_aabb(const Vec3 &u0, const Vec3 &u1, const Vec3 &u2, const Vec3 &n, 
							  const Vec3 &c, const Vec3 &h)
	{
		// �O�p�`�̒��_��AABB���S�����_�Ƃ������W�n�Ɉړ�
		vector<Vec3> v(3);
		v[0] = u0-c;
		v[1] = u1-c;
		v[2] = u2-c;

		//
		// 1. �O�p�`��AABB��AABB�̌�������
		//
		Vec3 bmin = -h,  bmax = h;
		Vec3 tmin = v[0], tmax = v[0];
		for(int j = 1; j < 3; ++j){
			for(int k = 0; k < 3; ++k){
				if(v[j][k] < tmin[k]) tmin[k] = v[j][k];
				if(v[j][k] > tmax[k]) tmax[k] = v[j][k];
			}
		}
		// �e���ł̏d�Ȃ蔻��
		int overlap = 0;
		for(int k = 0; k < 3; ++k){
			if(!(tmax[k] < bmin[k] || tmin[k] > bmax[k])){
				overlap++;
			}
		}
		// 3�����ׂĂŏd�Ȃ��Ă�����AABB���m���d�Ȃ��Ă���
		if(overlap < 3){
			return false;
		}

		// 
		// 2. AABB�ƎO�p�`���܂ޕ��ʂƂ̌�������
		//
		if(!plane_aabb(n, -dot(n, v[0]), Vec3(0.0), h)){
			return false;
		}

		// 
		// 3. AABB�̊e�������ƃG�b�W�̃e�X�g
		// 
		vector<Vec3> e(3, Vec3(0.0));	// AABB�̊e�������x�N�g��
		vector<Vec3> f(3);				// �O�p�`�̃G�b�W�x�N�g��
		for(int k = 0; k < 3; ++k){
			e[k][k] = 1.0;
			f[k] = v[(k == 2 ? 0 : k+1)]-v[k];
		}
		for(int i = 0; i < 3; ++i){
			for(int j = 0; j < 3; ++j){
				Vec3 a = cross(e[i], f[j]);
				Vec3 p = Vec3(dot(a, v[0]), dot(a, v[1]), dot(a, v[2]));
				double r = h[0]*fabs(a[0])+h[1]*fabs(a[1])+h[2]*fabs(a[2]);
				if(RX_MIN3(p[0], p[1], p[2]) > r || RX_MAX3(p[0], p[1], p[2]) < -r){
					return false;
				}
			}
		}

		return true;
	}

	/*!
	 * ���_���S�C�ӂ̒���1��AABB�ƃ|���S���̌�������
	 * @param[in] verts 
	 * @param[out] 
	 * @return 
	 */
	static int polygon_intersects_cube(const vector<Vec3> &verts, const Vec3 &p_nrm)
	{
		bool intersect = false;
		int vn = (int)verts.size();
		for(int i = 0; i < vn-2; ++i){
			if(triangle_aabb(verts[0], verts[i+1], verts[i+2], p_nrm, Vec3(0.0), Vec3(0.5))){
				intersect = true;
				break;
			}
		}

		return intersect;
	}

}

#endif

