/*!
  @file intersections.h

  @brief ��_�v�Z

  @author Makoto Fujisawa
  @date 2006
*/

#ifndef _RX_INTERSECTIONS_H_
#define _RX_INTERSECTIONS_H_


//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
#include "rx_utility.h"


//-----------------------------------------------------------------------------
// �萔
//-----------------------------------------------------------------------------
//! AABB�̊e�ʂ̖@��
const Vec3 RX_AABB_NORMALS[6] = { Vec3( 1.0,  0.0,  0.0), 
								  Vec3(-1.0,  0.0,  0.0), 
								  Vec3( 0.0,  1.0,  0.0), 
								  Vec3( 0.0, -1.0,  0.0), 
								  Vec3( 0.0,  0.0,  1.0), 
								  Vec3( 0.0,  0.0, -1.0) };

const Vec2 RX_AABB_NORMALS2[4] = { Vec2(-1.0,  0.0), 
								   Vec2( 1.0,  0.0), 
								   Vec2( 0.0, -1.0), 
								   Vec2( 0.0,  1.0) };

//-----------------------------------------------------------------------------
// ��_�v�Z�֐�
//-----------------------------------------------------------------------------
namespace RxIntersection
{
	/*!
	 * Franlin Antonio's gem�Ɋ�Â��G�b�W-�G�b�W�e�X�g
	 *  - "Faster Line Segment Intersection", in Graphics Gems III, pp.199-202
	 * @param[in] Ax,Ay
	 */
	inline int EDGE_EDGE_TEST(float &Ax, float &Ay, const int &i0, const int &i1, 
							  const Vec3 &V0, const Vec3 &U0, const Vec3 &U1)
	{
		float Bx = U0[i0]-U1[i0];
		float By = U0[i1]-U1[i1];
		float Cx = V0[i0]-U0[i0];
		float Cy = V0[i1]-U0[i1];
		float f = Ay*Bx-Ax*By;
		float d = By*Cx-Bx*Cy;
		if((f > 0 && d >= 0 && d <= f) || (f < 0 && d <= 0 && d >= f)){
			float e = Ax*Cy-Ay*Cx;
			if(f > 0){
				if(e >= 0 && e <= f) return 1;
			}
			else{
				if(e <= 0 && e >= f) return 1;
			}
		}

		return 0;
	}

	inline int EDGE_AGAINST_TRI_EDGES(const int &i0, const int &i1, 
									  const Vec3 &V0, const Vec3 &V1, 
									  const Vec3 &U0, const Vec3 &U1, const Vec3 &U2)
	{
		float Ax = V1[i0]-V0[i0];
		float Ay = V1[i1]-V0[i1];

		// �G�b�W(U0,U1)�ƃG�b�W(V0,V1)�̃e�X�g
		if(EDGE_EDGE_TEST(Ax, Ay, i0, i1, V0, U0, U1)) return 1;

		// �G�b�W(U1,U2)�ƃG�b�W(V0,V1)�̃e�X�g
		if(EDGE_EDGE_TEST(Ax, Ay, i0, i1, V0, U0, U1)) return 1;

		// �G�b�W(U2,U1)�ƃG�b�W(V0,V1)�̃e�X�g
		if(EDGE_EDGE_TEST(Ax, Ay, i0, i1, V0, U0, U1)) return 1;
	}


	inline int POINT_IN_TRI(const int &i0, const int &i1, 
							const Vec3 &V0, const Vec3 &U0, const Vec3 &U1, const Vec3 &U2)
	{
		float a, b, c, d0, d1, d2;

		// is T1 completly inside T2?
		// check if V0 is inside tri(U0,U1,U2)
		a = U1[i1]-U0[i1];
		b = -(U1[i0]-U0[i0]);
		c = -a*U0[i0]-b*U0[i1];
		d0 = a*V0[i0]+b*V0[i1]+c;

		a = U2[i1]-U1[i1];
		b = -(U2[i0]-U1[i0]);
		c = -a*U1[i0]-b*U1[i1];
		d1 = a*V0[i0]+b*V0[i1]+c;

		a = U0[i1]-U2[i1];
		b = -(U0[i0]-U2[i0]);
		c = -a*U2[i0]-b*U2[i1];
		d2 = a*V0[i0]+b*V0[i1]+c;

		if(d0*d1 > 0.0){
			if(d0*d2 > 0.0) return 1;
		}

		return 0;
	}


	inline bool compute_intervals(float v0, float v1, float v2, float d0, float d1, float d2, 
								  float d0d1, float d0d2, float &isect0, float &isect1)
	{
		if(d0d1 > 0.0f){		// d0, d1���������Cd2�����̑��ɂ���ꍇ
			isect0 = v2+(v0-v2)*d2/(d2-d0);
			isect1 = v2+(v1-v2)*d2/(d2-d1);

			return true;
		}
		else if(d0d2 > 0.0f){	// d0, d2���������Cd2�����̑��ɂ���ꍇ
			isect0 = v1+(v0-v1)*d1/(d1-d0);
			isect1 = v1+(v2-v1)*d1/(d1-d2);

			return true;
		}
		else if(d1*d2 > 0.0f || d0 != 0.0f){
			isect0 = v0+(v1-v0)*d0/(d0-d1);
			isect1 = v0+(v2-v0)*d0/(d0-d2);

			return true;
		}
		else if(d1 != 0.0f){
			isect0 = v1+(v0-v1)*d1/(d1-d0);
			isect1 = v1+(v2-v1)*d1/(d1-d2);

			return true;
		}
		else if(d2 != 0.0f){
			isect0 = v2+(v0-v2)*d2/(d2-d0);
			isect1 = v2+(v1-v2)*d2/(d2-d1);

			return true;
		}
		else{	// ���ꕽ��
			return false;
		}
	}

	// Triangle�����ꕽ�ʂɂ���Ƃ��̌�������
	inline bool coplanar_tri_tri(const Vec3 &n1, const Vec3 &u0, const Vec3 &u1, const Vec3 &u2, 
												 const Vec3 &v0, const Vec3 &v1, const Vec3 &v2)
	{
		Vec3 A;
		int i0,i1;
		// first project onto an axis-aligned plane, that maximizes the area
		// of the triangles, compute indices: i0,i1.
		A = RXFunc::Fabs(n1);
		if(A[0] > A[1]){
			if(A[0]>A[2]){
				i0=1;	  // A[0] is greatest
				i1=2;
			}
			else{
				i0=0;	  // A[2] is greatest
				i1=1;
			}
		}
		else{
			if(A[2]>A[1]){
				i0=0;	  // A[2] is greatest
				i1=1;										   
			}
			else{
				i0=0;	  // A[1] is greatest
				i1=2;
			}
		}			   
						
		// test all edges of triangle 1 against the edges of triangle 2
		EDGE_AGAINST_TRI_EDGES(i0, i1, u0, u1, v0, v1, v2);
		EDGE_AGAINST_TRI_EDGES(i0, i1, u1, u2, v0, v1, v2);
		EDGE_AGAINST_TRI_EDGES(i0, i1, u2, u0, v0, v1, v2);
					
		// finally, test if tri1 is totally contained in tri2 or vice versa
		if(POINT_IN_TRI(i0, i1, u0, v0, v1, v2)) return 1;
		if(POINT_IN_TRI(i0, i1, v0, u0, u1, u2)) return 1;

		return false;
	}

	/*!
	   MARK:�O�p�`�ƎO�p�`�̌�������
	 *  - Moller, Tomas, "A Fast Triangle-Triangle Intersection Test"
	 *	http://www.acm.org/jgt/papers/Moller97/
	 * @param[in] u0,u1,u2	�O�p�`1�̒��_
	 * @param[in] n1		�O�p�`1�̖@��
	 * @param[in] v0,v1,v2	�O�p�`2�̒��_
	 * @param[in] n2		�O�p�`2�̖@��
	 * @return 
	 */
	static bool triangle_triangle(const Vec3 &u0, const Vec3 &u1, const Vec3 &u2, const Vec3 &n1, 
								  const Vec3 &v0, const Vec3 &v1, const Vec3 &v2, const Vec3 &n2)
	{
		//Vec3 n1, n2;	// Triangle�̖@��
		float d1, d2;	// Triangle��̓_(Triangle���܂ޕ���p=n*x+d)
		float du0, du1, du2, dv0, dv1, dv2;	// ���ʂƊe���_�̋���
		Vec3 D;			// �������̕���
		float isect1[2], isect2[2];
		float du0du1, du0du2, dv0dv1, dv0dv2;
		float up0, up1, up2;	// ��������(u0, u1, u2)���ˉe�����ʒu
		float vp0, vp1, vp2;	// ��������(v0, v1, v2)���ˉe�����ʒu
		int index;
		float b, c, max;

		// Triangle 1�̕��ʂ̕������̌v�Z
		//n1 = Unit(cross(u1-u0, u2-u0);
		d1 = -dot(n1, u0);

		// (v0, v1, v2)��Triangle 1�̕��ʂƂ̕����t�������̌v�Z
		dv0 = dot(n1, v0)+d1;
		dv1 = dot(n1, v1)+d1;
		dv2 = dot(n1, v2)+d1;

		if(fabs(dv0) < RX_EPS) dv0 = 0.0;
		if(fabs(dv1) < RX_EPS) dv1 = 0.0;
		if(fabs(dv2) < RX_EPS) dv2 = 0.0;

		dv0dv1 = dv0*dv1;
		dv0dv2 = dv0*dv2;

		// (dv0, dv1, dv2)���S�ē��������ŁC0�łȂ��Ȃ�Ό����Ȃ�
		if(dv0dv1 > 0.0f && dv0dv2 > 0.0f)
			return false;


		// Triangle 2�̕��ʂ̕������̌v�Z
		//n2 = Unit(cross(v1-v0, v2-v0);
		d2 = -dot(n2, v0);

		// (u0, u1, u2)��Triangle 2�̕��ʂƂ̕����t�������̌v�Z
		du0 = dot(n2, u0)+d2;
		du1 = dot(n2, u1)+d2;
		du2 = dot(n2, u2)+d2;

		if(fabs(du0) < RX_EPS) du0 = 0.0;
		if(fabs(du1) < RX_EPS) du1 = 0.0;
		if(fabs(du2) < RX_EPS) du2 = 0.0;

		du0du1 = du0*du1;
		du0du2 = du0*du2;

		// (du0, du1, du2)���S�ē��������ŁC0�łȂ��Ȃ�Ό����Ȃ�
		if(du0du1 > 0.0f && du0du2 > 0.0f)
			return false;


		// �������̕������v�Z
		D = cross(n1, n2);

		// D�̍ł��傫���v�f�̔���
		max = fabs(D[0]);
		index = 0;
		b = fabs(D[1]);
		c = fabs(D[2]);
		if(b > max){
			max = b;
			index = 1;
		}
		if(c > max){
			max = c;
			index = 2;
		}

		// �������ւ̎ˉe����
		up0 = u0[index];
		up1 = u1[index];
		up2 = u2[index];

		vp0 = v0[index];
		vp1 = v1[index];
		vp2 = v2[index];


		// Triangle 1��Interval�̌v�Z
		if(!compute_intervals(up0, up1, up2, du0, du1, du2, du0du1, du0du2, isect1[0], isect1[1])){
			// ���ꕽ�ʎ��̏���
			return coplanar_tri_tri(n1, u0, u1, u2, v0, v1, v2);
		}

		// Triangle 2��Interval�̌v�Z
		if(!compute_intervals(vp0, vp1, vp2, dv0, dv1, dv2, dv0dv1, dv0dv2, isect2[0], isect2[1])){
			// ���ꕽ�ʎ��̏���
			return coplanar_tri_tri(n1, u0, u1, u2, v0, v1, v2);
		}

		float tmp;
		if(isect1[0] > isect1[1]){
			tmp = isect1[0];
			isect1[0] = isect1[1];
			isect1[1] = tmp;
		}
		if(isect2[0] > isect2[1]){
			tmp = isect2[0];
			isect2[0] = isect2[1];
			isect2[1] = tmp;
		}

		if(isect1[1] < isect2[0] || isect2[1] < isect1[0])
			return false;


		return true;
	}

	/*!
	   MARK:�����ƕ��ʂ̌�������
	 *  - ���� : start+dir*t (start�`start+dir)
	 *	���� : n�Ex+d = 0
	 * @param[in] start	: �����̎��_
	 * @param[in] dir		: �����̕���(���K���̕K�v�Ȃ�)
	 * @param[in] pn		: ���ʖ@��
	 * @param[in] pd		: 
	 * @param[in] fraction	: �����ƕ��ʂ̌�_�܂ł̋�����Ԃ�
	 * @return ���������ʂƌ������Ă����ꍇ�Ftrue
	 */
	static bool line_plane(const Vec3 &start, const Vec3 &dir, Vec3 &pn, float pd, float &fraction )
	{
		float sc, len;

		sc = dot(start, pn)+pd;	// ���ʂ�start�̕����t������
		if(sc < 0){		// �o���_�����ʂ̗��̃p�^�[��
			return false;
		}

		len = -dot(dir, pn);
		if(len < sc){
			return false;
		}

		fraction = sc/len;
		return true;
	}

	/*!
	   MARK:���ƕ��ʂ̌�������
	 *  -  �� : (center-x)^2=radius^2, ���� : pn�Ex+pd = 0
	 * @param[in] c		: ���̒��S���W
	 * @param[in] tv	: �����^�C���X�e�b�v�ԂɌ���������(���K���Ȃ�)
	 * @param[in] r		: ���̔��a
	 * @param[in] pn	: ���ʖ@��
	 * @param[in] pd	: pd = -pn�Ex
	 * @param[out] t	: ���ʂɏՓ˂���܂ł̃p�����[�^����
	 * @return �������ʂƌ������Ă����ꍇ�Ftrue
	 */
	static bool sphere_plane(const Vec3 &c, const Vec3 &tv, float r, Vec3 &pn, float &pd, float &t)
	{
		float sc, se;
		Vec3 e = c+tv;

		sc = dot(c, pn)+pd;	// ���ʂƃ^�C���X�e�b�v�n�߂̋�(�n��)�̕����t������
		se = dot(e, pn)+pd;	// ���ʂƃ^�C���X�e�b�v�I���̋�(�I��)�̕����t������

		// �n���ƏI�������ʂ̓������ɂ���C���S�ƕ��ʂ̋��������a�ȏ�̏ꍇ�C�����Ȃ�
		if(sc*se > 0 && (fabs(sc) > r && fabs(se) > r)){
			return false;
		}

		t = (sc-r)/(sc-se);

		return true;
	}

	/*!
	   MARK:�~��2�����Z���̌�������
	 * @param[in] vMax		�Z���E����W
	 * @param[in] vMin		�Z���������W
	 * @param[in] vCenter	�~���S
	 * @param[in] fRadius	�~���a
	 * @return ��������
	 */
	static bool circle_cell(const Vec2 &vMin, const Vec2 &vMax, const Vec2 &vCenter, const float &fRadius)
	{
		// �~���S���Z�����ɂ���ꍇ
		if(vMax[0] > vCenter[0] && vMax[1] > vCenter[1] && vMin[0] < vCenter[0] && vMin[1] < vCenter[1]){
			return true;
		}

		// �Z���̕ӂƉ~���d�Ȃ��Ă���ꍇ
		if(vCenter[0] < vMax[0] && vCenter[0] > vMin[0]){
			if(fabs(vCenter[1]-vMin[1]) < fRadius) return true;
			if(fabs(vCenter[1]-vMax[1]) < fRadius) return true;
		}
		if(vCenter[1] < vMax[1] && vCenter[1] > vMin[1]){
			if(fabs(vCenter[0]-vMin[0]) < fRadius) return true;
			if(fabs(vCenter[0]-vMax[0]) < fRadius) return true;
		}

		// �Z����4���_�̂����ЂƂł��~���ɂ���ꍇ
		float fRad2 = fRadius*fRadius;
		if(norm2(vMax-vCenter) < fRad2) return true;	// �E��_
		if(norm2(vMin-vCenter) < fRad2) return true;	// �����_
		if(norm2(Vec2(vMax[0]-vCenter[0], vMin[1]-vCenter[1])) < fRad2) return true;	// �E���_
		if(norm2(Vec2(vMin[0]-vCenter[0], vMax[1]-vCenter[1])) < fRad2) return true;	// ����_

		return false;
	}

	/*!
	 * �Z�ʑ̂̕��ʂƌ����̌�������
	 * @param[in] ray_org,ray_dir �����̌��_�ƕ��� 
	 * @param[in] box_min,box_max �Z�ʑ̂̍ŏ��C�ő���W
	 * @param[in] planeVec ����
	 * @param[in] axis ��
	 * @param[out] dist ��_�܂ł̋���
	 * @return ������true�C�������Ȃ�or���_�̌��Ō�����false
	 */
	static bool ray_boxplane(const Vec3 &ray_org, const Vec3 &ray_dir, 
							 const Vec3 &box_min, const Vec3 &box_max, 
							 const Vec3 &planeVec, int axis, double &dist)
	{
		dist = (planeVec[axis]-ray_org[axis])/ray_dir[axis];
		if(dist > 0.0f){
			Vec3 intersect = ray_org+ray_dir*dist;
			for(int i = 0; i < 3; ++i){
				if( axis != i && (intersect[i] < box_min[i] || intersect[i] > box_max[i]) ){
					return false;
				}
			}
			return true;
		}
		else{
			// ���_�̌��
			return false;
		}
	}

	/*!
	 * Which of the six face-plane(s) is point P outside of? 
	 * @param 
	 * @return 
	 */
	static int FacePlane(const Vec3 p)
	{
		int outcode;

		outcode = 0;
		if(p[0] >  0.5)	outcode |= 0x01;
		if(p[0] < -0.5)	outcode |= 0x02;
		if(p[1] >  0.5) outcode |= 0x04;
		if(p[1] < -0.5) outcode |= 0x08;
		if(p[2] >  0.5) outcode |= 0x10;
		if(p[2] < -0.5) outcode |= 0x20;
	   
		return outcode;
	}

	/*!
	 * 
	 * @param 
	 * @return 
	 */
	static int FacePlane(const Vec3 p, const float &sl)
	{
		int outcode = 0;

		if(p[0] >  sl) outcode |= 0x01;
		if(p[0] < -sl) outcode |= 0x02;
		if(p[1] >  sl) outcode |= 0x04;
		if(p[1] < -sl) outcode |= 0x08;
		if(p[2] >  sl) outcode |= 0x10;
		if(p[2] < -sl) outcode |= 0x20;
	   
		return outcode;
	}

	/*!
	 * �_�������̓��ɂ��邩�ǂ����𔻕�
	 * @param[in] p   �_
	 * @param[in] sl  �����̂̈�ӂ̒����̔���
	 * @param[in] cen �����̂̒��S���W
	 * @return �����̂ɂ����true
	 */
	static bool InCubePoint(const Vec3 p, const float sl = 0.5, const Vec3 cen = Vec3(0.0, 0.0, 0.0))
	{
		Vec3 p0 = p-cen;

		if(p0[0] >  sl) return false;
		if(p0[0] < -sl) return false;
		if(p0[1] >  sl) return false;
		if(p0[1] < -sl) return false;
		if(p0[2] >  sl) return false;
		if(p0[2] < -sl) return false;
	   
		return true;
	}

	/*!
	 * �_�������̓��ɂ��邩�ǂ����𔻕�
	 * @param[in] p   �_
	 * @param[in] len �����̂̕ӂ̒���
	 * @param[in] cen �����̂̒��S���W
	 * @return �����̓��ɂ����true
	 */
	static bool InRectPoint3D(const Vec3 p, const Vec3 len = Vec3(1.0), const Vec3 cen = Vec3(0.0))
	{
		Vec3 p0 = p-cen;

		if(p0[0] >  0.5*len[0]) return false;
		if(p0[0] < -0.5*len[0]) return false;
		if(p0[1] >  0.5*len[1]) return false;
		if(p0[1] < -0.5*len[1]) return false;
		if(p0[2] >  0.5*len[2]) return false;
		if(p0[2] < -0.5*len[2]) return false;
	   
		return true;
	}

	/*!
	 * �_�������̓��ɂ��邩�ǂ����𔻕�
	 * @param[in] p	�_
	 * @param[in] _min �����̂̍ŏ��l
	 * @param[in] _max �����̂̍ő�l
	 * @return �����̓��ɂ����true
	 */
	static bool InRectPoint3DM(const Vec3 p, const Vec3 _min = Vec3(-0.5), const Vec3 _max = Vec3(0.5))
	{
		if(p[0] > _max[0]) return false;
		if(p[0] < _min[0]) return false;
		if(p[1] > _max[1]) return false;
		if(p[1] < _min[1]) return false;
		if(p[2] > _max[2]) return false;
		if(p[2] < _min[2]) return false;
	   
		return true;
	}

	/*!
	   MARK:�����ƘZ�ʑ̂̌�������
	 * @param[in] ray_org,ray_dir �����̌��_�ƕ��� 
	 * @param[in] box_min,box_max �Z�ʑ̂̍ŏ��C�ő���W
	 * @param[out] dist ��_�܂ł̋���
	 * @return ������true�C�������Ȃ�or���_�̌��Ō�����false
	 */
	static bool ray_box(const Vec3 &ray_org, const Vec3 &ray_dir, 
						const Vec3 &box_min, const Vec3 &box_max, double &dist)
	{
		if(InRectPoint3DM(ray_org, box_min, box_max)){	// ���_��Box���ɂ���Ƃ�
			for(int i = 0; i < 3; ++i){
				Vec3 planeVec;
				if(ray_dir[i] > RX_FEQ_EPS){
					planeVec = box_max;
				}
				else{
					planeVec = box_min;
				}

				if((planeVec != 0) && ray_boxplane(ray_org, ray_dir, box_min, box_max, planeVec, i, dist)){
					return true;
				}
			}
		}
		else{	// ���_��Box�O�ɂ���Ƃ�
			for(int i = 0; i < 3; ++i){
				Vec3 planeVec;
				if(ray_dir[i] > RX_FEQ_EPS){
					planeVec = box_min;
				}
				else{
					planeVec = box_max;
				}

				if((planeVec != 0) && ray_boxplane(ray_org, ray_dir, box_min, box_max, planeVec, i, dist)){
					return true;
				}
			}
		}
		return false;
	}

	/*!
	 * �Z�ʑ̂̌����̌�������(Woo's method)
	 *  - Real-Time Rendering, pp575-576 or Woo, Andrew, "Fast Ray-Box Intersection," Graphics Gems pp395-396
	 * @param[in] ray_org,ray_dir �����̌��_�ƕ��� 
	 * @param[in] box_min,box_max �Z�ʑ̂̍ŏ��C�ő���W
	 * @return ������true�C�������Ȃ�or���_�̌��Ō�����false
	 */
	static bool ray_box_by_woo(const Vec3 &ray_org, const Vec3 &ray_dir, 
							   const Vec3 &box_min, const Vec3 &box_max, Vec3 &coord)
	{
		bool inside = true;
		int quadrant[3]; // RIGHT:0, LEFT:1, MIDDLE:2
		register int i;
		int whichPlane;
		Vec3 maxT;
		Vec3 candidatePlane;

		// Find candidate planes; this loop can be avoided if rays cast all from the eye(assume perpsective view)
		for(i = 0; i < 3; ++i){
			if(ray_org[i] < box_min[i]){
				quadrant[i] = 1;
				candidatePlane[i] = box_min[i];
				inside = false;
			}
			else if(ray_org[i] > box_max[i]){
				quadrant[i] = 0;
				candidatePlane[i] = box_max[i];
				inside = false;
			}
			else{
				quadrant[i] = 2;
			}
		}

		// Ray origin inside bounding box
		if(inside){
			coord = ray_org;
			return true;
		}


		// Calculate T distances to candidate planes
		for(i = 0; i < 3; ++i){
			if(quadrant[i] != 2 && ray_dir[i] != 0.0){
				maxT[i] = (candidatePlane[i]-ray_org[i])/ray_dir[i];
			}
			else{
				maxT[i] = -1.0;
			}
		}

		// Get largest of the maxT's for final choice of intersection
		whichPlane = 0;
		for(i = 1; i < 3; ++i){
			if(maxT[whichPlane] < maxT[i]){
				whichPlane = i;
			}
		}

		// Check final candidate actually inside box
		if(maxT[whichPlane] < 0.0)
			return false;

		for(i = 0; i < 3; ++i){
			if(whichPlane != i){
				coord[i] = ray_org[i] + maxT[whichPlane]*ray_dir[i];
				
				if(coord[i] < box_min[i] || coord[i] > box_max[i])
					return false;
			}
			else{
				coord[i] = candidatePlane[i];
			}
		}

		return true;
	}


	/*!
	 * ���C/�����ƎO�p�`�̌���
	 * @param[in] P0,P1 ���C/�����̒[�_or���C��̓_
	 * @param[in] V0,V1,V2 �O�p�`�̒��_���W
	 * @param[out] I ��_���W
	 * @retval 1 ��_I�Ō��� 
	 * @retval 0 ��_�Ȃ�
	 * @retval 2 �O�p�`�̕��ʓ�
	 * @retval -1 �O�p�`��"degenerate"�ł���(�ʐς�0�C�܂�C�������_�ɂȂ��Ă���)
	 */
	static int intersectSegmentTriangle(Vec3 P0, Vec3 P1,			// Segment
										Vec3 V0, Vec3 V1, Vec3 V2,	// Triangle
										Vec3 &I)					// Intersection point (return)
	{
		Vec3 u, v, n;
		Vec3 dir, w;
		float r, a, b;

		// �O�p�`�̃G�b�W�x�N�g���Ɩ@��
		u = V1-V0;		
		v = V2-V0;			
		n = cross(u, v);
		if(RXFunc::IsZeroVec(n)){
			return -1;	// �O�p�`��"degenerate"�ł���(�ʐς�0)
		}

		// ����
		dir = P1-P0;
		a = -dot(n, P0-V0);
		b = dot(n, dir);
		if(fabs(b) < RX_FEQ_EPS){	// �����ƎO�p�`���ʂ����s
			if(a == 0){
				return 2;	// ���������ʏ�
			}
			else{
				return 0;	// ��_�Ȃ�
			}
		}

		// ��_�v�Z
		r = a/b;
		if(r < 0.0 || r > 1.0){ // ���C���ΏۂȂ�"r > 1.0"�͕K�v�Ȃ�
			return 0;
		}

		// �����ƕ��ʂ̌�_
		I = P0+r*dir;

		// ��_���O�p�`���ɂ��邩�ǂ����̔���
		float uu, uv, vv, wu, wv, D;
		uu = dot(u, u);
		uv = dot(u, v);
		vv = dot(v, v);
		w = I-V0;
		wu = dot(w, u);
		wv = dot(w, v);
		D = uv*uv-uu*vv;

		float s, t;
		s = (uv*wv-vv*wu)/D;
		if(s < 0.0 || s > 1.0){
			return 0;
		}
		
		t = (uv*wu-uu*wv)/D;
		if(t < 0.0 || (s+t) > 1.0){
			return 0;
		}

		return 1;
	}

	/*!
	 * �����ƎO�p�`�̌���
	 * @param[in] P0,P1 ���C/�����̒[�_or���C��̓_
	 * @param[in] V0,V1,V2 �O�p�`�̒��_���W
	 * @param[out] I ��_���W
	 * @retval 2 �����[�_�Ō���
	 * @retval 1 ��_I�Ō��� 
	 * @retval 0 ��_�Ȃ�
	 * @retval -1 �O�p�`��"degenerate"�ł���(�ʐς�0�C�܂�C�������_�ɂȂ��Ă���)
	 * @retval -2 �O�p�`�̕��ʓ�
	 */
	static int segment_triangle(Vec3 P0, Vec3 P1, Vec3 V0, Vec3 V1, Vec3 V2,
								Vec3 &I, double &len)
	{
		Vec3 u, v, n;
		Vec3 dir, w;
		double r, a, b;

		// �O�p�`�̃G�b�W�x�N�g���Ɩ@��
		u = V1-V0;		
		v = V2-V0;			
		n = cross(u, v);
		if(RXFunc::IsZeroVec(n)){
			return -1;	// �O�p�`��"degenerate"�ł���(�ʐς�0)
		}

		// ����
		dir = P1-P0;
		a = -dot(n, P0-V0);
		b = dot(n, dir);
		if(fabs(b) < RX_FEQ_EPS){	// �����ƎO�p�`���ʂ����s
			if(a == 0){
				return -2;	// ���������ʏ�
			}
			else{
				return 0;	// ��_�Ȃ�
			}
		}

		// ��_�v�Z
		r = a/b;
		if(r < 0.0 || r > 1.0){ // ���C���ΏۂȂ�"r > 1.0"�͕K�v�Ȃ�
			return 0;
		}

		// �����ƕ��ʂ̌�_
		I = P0+r*dir;

		// ��_���O�p�`���ɂ��邩�ǂ����̔���
		float uu, uv, vv, wu, wv, D;
		uu = dot(u, u);
		uv = dot(u, v);
		vv = dot(v, v);
		w = I-V0;
		wu = dot(w, u);
		wv = dot(w, v);
		D = uv*uv-uu*vv;

		float s, t;
		s = (uv*wv-vv*wu)/D;
		if(s < 0.0 || s > 1.0){
			return 0;
		}
		
		t = (uv*wu-uu*wv)/D;
		if(t < 0.0 || (s+t) > 1.0){
			return 0;
		}

		len = r;
		if(r <= RX_FEQ_EPS || r >= 1.0-RX_FEQ_EPS){
			return 2;
		}
		
		return 1;
	}


	/*!
	   MARK:�_�ƕ��ʂ̋���
	 * @param[in] v  �_�̍��W
	 * @param[in] pn ���ʖ@��(�p�����[�^a,b,c)
	 * @param[in] pd ���ʃp�����[�^d
	 * @return ����
	 */
	static double point_plane_dist(const Vec3 &v, const Vec3 &pn, const double &pd)
	{
		return (dot(v, pn)+pd)/norm(pn);
	}

	/*!
	   MARK:�_�ƕ��ʂ̋���
	 * @param[in] v  �_�̍��W
	 * @param[in] px ���ʏ�̓_
	 * @param[in] pn ���ʂ̖@��
	 * @return ����
	 */
	static double point_plane_dist(const Vec3 &v, const Vec3 &px, const Vec3 &pn)
	{
		return dot((v-px), pn)/norm(pn);
	}

	/*!
	   MARK:�O�p�`�Ɠ_�̋����ƍŋߖT�_
	 * @param[in] v0,v1,v2	�O�p�`�̒��_
	 * @param[in] n			�O�p�`�̖@��
	 * @param[in] p			�_
	 * @return 
	 */
	inline bool triangle_point_dist(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2, const Vec3 &n, 
									const Vec3 &p, double &dist, Vec3 &p0)
	{
		// �|���S�����܂ޕ��ʂƓ_�̋���
		double l = point_plane_dist(p, v0, n);
		
		// ���ʂƂ̍ŋߖT�_���W
		Vec3 np = p-l*n;

		// �ߖT�_���O�p�`�����ǂ����̔���
		Vec3 n1 = cross((v0-p), (v1-p));
		Vec3 n2 = cross((v1-p), (v2-p));
		Vec3 n3 = cross((v2-p), (v0-p));

		if(dot(n1, n2) > 0 && dot(n2, n3) > 0){
			// �O�p�`��
			dist = l;
			p0 = np;
			return true;
		}
		else{
			// �O�p�`�O
			return false;
		}
	}
			
			
	/*!
	 * �������m�̋���
	 * @param[in] a1,a3 ����1�̗��[�_
	 * @param[in] b1,b3 ����2�̗��[�_
	 * @param[in] v
	 * @return ����
	 */
	inline double segment_segment_dist(Vec3 a1, Vec3 a3, Vec3 b1, Vec3 b3, Vec3 &v)
	{
		Vec3 a2 = (a1+a3)*0.5, b2 = (b1+b3)*0.5;

		double d11 = norm2(a1-b1);
		double d12 = norm2(a1-b2);
		double d13 = norm2(a1-b3);
		double d21 = norm2(a2-b1);
		double d22 = norm2(a2-b2);
		double d23 = norm2(a2-b3);
		double d31 = norm2(a3-b1);
		double d32 = norm2(a3-b2);
		double d33 = norm2(a3-b3);

		double d = d11;
		Vec3 pa = a1, pb = b1;
		if(d12 < d){ d = d12; pa = a1; pb = b2; }
		if(d13 < d){ d = d13; pa = a1; pb = b3; }
		if(d21 < d){ d = d21; pa = a2; pb = b1; }
		if(d22 < d){ d = d22; pa = a2; pb = b2; }
		if(d23 < d){ d = d23; pa = a2; pb = b3; }
		if(d31 < d){ d = d31; pa = a3; pb = b1; }
		if(d32 < d){ d = d32; pa = a3; pb = b2; }
		if(d33 < d){ d = d33; pa = a3; pb = b3; }

		v = pb-pa;
		d = sqrt(d);
		v /= d;

		return d;
	}
	/*!
	 * �����̂Ɠ_�̋���
	 * @param[in] spos �����̂̒��S�����_�Ƃ������΍��W�l
	 * @param[in] r	���a(���̏ꍇ)
	 * @param[in] sgn  �����̂̓��ŋ�������:1,�O�Ő�:-1
	 * @param[in] vMin �����̂̍ŏ����W�l(���΍��W)
	 * @param[in] vMax �����̂̍ő���W�l(���΍��W)
	 * @param[out] d   �����t�����l
	 * @param[out] n   �ŋߖT�_�̖@������
	 * @return 
	 */
	inline bool AABB_point_dist(const Vec3 &spos, const double &r, const int &sgn, 
								const Vec3 &vMin, const Vec3 &vMax, 
								double &d, Vec3 &n)
	{
		int bout = 0;
		double d0[6];
		int idx0 = -1;

		// �e�����Ƃɍŏ��ƍő勫�E�O�ɂȂ��Ă��Ȃ������ׂ�
		int c = 0;
		for(int i = 0; i < 3; ++i){
			int idx = 2*i;
			if((d0[idx] = (spos[i]-r)-vMin[i]) < 0.0){
				bout |= (1 << idx); c++;
				idx0 = idx;
			}
			idx = 2*i+1;
			if((d0[idx] = vMax[i]-(spos[i]+r)) < 0.0){
				bout |= (1 << idx); c++;
				idx0 = idx;
			}
		}

		// AABB��(�S���ŋ��E��)
		if(bout == 0){
			double min_d = 1e10;
			int idx1 = -1;
			for(int i = 0; i < 6; ++i){
				if(d0[i] < min_d){
					min_d = d0[i];
					idx1 = i;
				}
			}

			d = sgn*min_d;
			n = (idx1 != -1) ? sgn*RX_AABB_NORMALS[idx1] : Vec3(0.0);
			return true;
		}


		// AABB�O
		Vec3 x(0.0);
		for(int i = 0; i < 3; ++i){
			if(bout & (1 << (2*i))){
				x[i] = d0[2*i];
			}
			else if(bout & (1 << (2*i+1))){
				x[i] = -d0[2*i+1];
			}
		}

		// sgn = 1:���C-1:�I�u�W�F�N�g
		if(c == 1){
			// ���ʋߖT
			d = sgn*d0[idx0];
			n = sgn*RX_AABB_NORMALS[idx0];
		}
		else{
			// �G�b�W/�R�[�i�[�ߖT
			d = -sgn*norm(x);
			n = sgn*(-Unit(x));
		}

		return false;
	}
	/*!
	 * �����̂Ɠ_�̕����t����(�����̓��Ȃ狗���l����)
	 * @param[in] spos �����̂̒��S�����_�Ƃ������΍��W�l
	 * @param[in] r	���a(���̏ꍇ)
	 * @param[in] vMin �����̂̍ŏ����W�l(���΍��W)
	 * @param[in] vMax �����̂̍ő���W�l(���΍��W)
	 * @param[out] d   �����t�����l
	 * @param[out] n   �ŋߖT�_�̖@������
	 */
	inline void AABB_point_dist2(const Vec2 &spos, const double &r, 
								 const Vec2 &vMin, const Vec2 &vMax, 
								 double &d, Vec2 &n)
	{
		int bout = 0;
		double d0[4];
		int idx0 = -1;

		// �e�����Ƃ�spos���ŏ��ƍő勫�E�O�ɂȂ��Ă��Ȃ������ׂ�
		int c = 0;
		for(int i = 0; i < 2; ++i){
			int idx = 2*i;
			if((d0[idx] = (spos[i]-r)-vMin[i]) < 0.0){
				bout |= (1 << idx); c++;
				idx0 = idx;
			}
			idx = 2*i+1;
			if((d0[idx] = vMax[i]-(spos[i]+r)) < 0.0){
				bout |= (1 << idx); c++;
				idx0 = idx;
			}
		}

		// AABB��(�S���ŋ��E��)
		if(bout == 0){
			double min_d = 1e10;
			int idx1 = -1;
			// �ł��߂��ӂ�T��
			for(int i = 0; i < 4; ++i){
				if(d0[i] < min_d){
					min_d = d0[i];
					idx1 = i;
				}
			}

			d = -min_d;
			n = (idx1 != -1) ? RX_AABB_NORMALS2[idx1] : Vec2(0.0);
			return;
		}

		// AABB�O
		Vec2 x(0.0);
		for(int i = 0; i < 2; ++i){
			if(bout & (1 << (2*i))){
				x[i] = d0[2*i];
			}
			else if(bout & (1 << (2*i+1))){
				x[i] = -d0[2*i+1];
			}
		}

		// sgn = 1:���C-1:�I�u�W�F�N�g
		if(c == 1){
			// �G�b�W�ߖT
			d = fabs(d0[idx0]);
			n = RX_AABB_NORMALS2[idx0];
		}
		else{
			// �R�[�i�[�ߖT
			d = norm(x);
			n = Unit(x);
		}

		return;
	}

	/*!
	 * �������m�̌�������(2D)
	 *  - Graphics Gems III pp.199-202 "Faster Line Segment Intersection"
	 * @param[in] A,B ����1�̗��[�_���W
	 * @param[in] C,D ����2�̗��[�_���W
	 * @param[out] P ��_���W
	 * @return �������Ă����ꍇ�F1, ���꒼���� : 2, �������Ȃ� : 0
	 */
	static int line_line_segment(const Vec2 &A, const Vec2 &B, const Vec2 &C, const Vec2 &D, Vec2 &P)
	{
		double Ax, Bx, Cx, Ay, By, Cy;
		double x1lo, x1hi, y1lo, y1hi;

		Ax = B[0]-A[0];
		Bx = C[0]-D[0];

		// X�����̗�������BBox�e�X�g
		if(Ax < 0){
			x1lo = B[0]; x1hi = A[0];
		}
		else{
			x1hi = B[0]; x1lo = A[0];
		}
		if(Bx > 0){
			if(x1hi < D[0] || C[0] < x1lo) return 0;
		} 
		else{
			if(x1hi < C[0] || D[0] < x1lo) return 0;
		}

		Ay = B[1]-A[1];
		By = C[1]-D[1];

		// Y�����̗�������BBox�e�X�g
		if(Ay < 0){
			y1lo = B[1]; y1hi = A[1];
		}
		else{
			y1hi = B[1]; y1lo = A[1];
		}
		if(By > 0){
			if(y1hi < D[1] || C[1] < y1lo) return 0;
		}
		else{
			if(y1hi < C[1] || D[1] < y1lo) return 0;
		}

		Cx = A[0]-C[0];
		Cy = A[1]-C[1];
		double num_t = By*Cx-Bx*Cy;	// t numerator
		double denom = Ay*Bx-Ax*By;	// both denominator
		if(denom > 0){			// t tests
			if(num_t < 0 || num_t > denom) return 0;
		}
		else{
			if(num_t > 0 || num_t < denom) return 0;
		}

		double num_s = Ax*Cy-Ay*Cx;	// s numerator
		if(denom > 0){			// s tests
			if(num_s < 0 || num_s > denom) return 0;
		}
		else{
			if(num_s > 0 || num_s < denom) return 0;
		}

		// ��_���W�̌v�Z
		if(RX_FEQ(denom, 0.0)) return 2;	// ���꒼����

		double num, offset;
		num = num_t*Ax;
		offset = (num*denom > 0.0) ? denom/2 : -denom/2;
		P[0] = A[0]+(num+offset)/denom;

		num = num_t*Ay;
		offset = (num*denom > 0.0) ? denom/2 : -denom/2;
		P[1] = A[1]+(num+offset)/denom;

		return 1;
	}

	/*!
	 * �����Ɖ~�̌�������(2D)
	 * @param[in] A,B �����̗��[�_���W
	 * @param[in] C �~�̒��S
	 * @param[in] r �~�̔��a
	 * @param[out] P ��_���W
	 * @return ��_��
	 */
	static int line_circle(const Vec2 &A, const Vec2 &B, const Vec2 &C, const double &r, Vec2 P[2])
	{
		double rr = r*r;
		Vec2 AC = C-A;
		Vec2 BC = C-B;

		Vec2 v = B-A;
		double l = norm(v);
		v /= l;

		double td = dot(v, AC);
		Vec2 D = A+td*v;
		double dd = norm2(D-C);

		if(dd < rr){
			double dt = sqrt(rr-dd);

			double da = rr-norm2(AC);
			double db = rr-norm2(BC);

			int inter = 0;
			double t1 = td-dt;
			double t2 = td+dt;
			if(t1 >= 0 && t1 <= l){
				P[inter] = A+t1*v;
				inter++;
			}
			if(t2 >= 0 && t2 <= l){
				P[inter] = A+t2*v;
				inter++;
			}

			return inter;
		}
		else{
			return 0;
		}
	}

	
	/*!
	 * 3�_��1������ɑ��݂��邩�`�F�b�N
	 * @param[in] p0,p1,p2 3���_���W�l
	 * @retval true  3�_��1������ɑ��݂���
	 * @retval false 3�_��1������ɑ��݂��Ȃ�
	 */
	inline bool IsOnLine(const Vec3 &p0, const Vec3 &p1, const Vec3 &p2)
	{
		// �x�N�g���̎Z�o
		Vec3 temp1 = p0-p1;
		Vec3 temp2 = p1-p2;

		// �O�ϒl��0�Ȃ��3�_��1������ɑ���
		if(cross(temp1, temp2) == Vec3(0.0, 0.0, 0.0) ){
			return true;
		}
		else{
			return false;
		}
	}

	/*!
	 * ����_(pos)��(ep1��ep2�ō\�����ꂽ)�G�b�W�ɑ΂��āC
	 * �G�b�W�ɂȂ������O�p�`�̎c��̒��_(tri)�̔��Α��ɂ��邩�ǂ������������D
	 * @param[in] ep1,ep2 �G�b�W���\�����钸�_���W
	 * @param[in] tri �O�p�`�̎c��̒��_���W
	 * @param[in] pos ���ׂ������_���W
	 * @retval true  ���Α��ɂ���
	 * @retval false �������ɂ���
	 */
	inline bool IsOtherSide(const Vec3 &ep1, const Vec3 &ep2, const Vec3 &tri, const Vec3 &pos)
	{
		Vec3 edge = ep1-ep2;	// ��G�b�W�x�N�g��
		Vec3 to_pos = pos-ep2;	// �ߖT�_�ւ̃x�N�g��
		Vec3 to_tri = tri-ep2;	// �O�p�`��1�̒��_�ւ̃x�N�g��

		// �ߖT�x�N�g���ƒ��_�x�N�g���̐����p
		double ang = dot(Unit(cross(edge, to_pos)), Unit(cross(edge, to_tri)));

		if(ang < 0.0){	// �ߖT�_���P�̒��_�ɑ΂���Edge�̔��Α��ɑ���
			return true;
		}
		else{
			return false;
		}
	}

	/*!
	 * ����_(pos)��(ep1��ep2�ō\�����ꂽ)�G�b�W�ɑ΂��āC
	 * �G�b�W�ɂȂ������O�p�`�̎c��̒��_(tri)�̔��Α��ɂ��邩�ǂ������������D
	 * @param[in] ep1,ep2 �G�b�W���\�����钸�_���W
	 * @param[in] tri �O�p�`�̎c��̒��_���W
	 * @param[in] pos ���ׂ������_���W
	 * @retval true  ���Α��ɂ���
	 * @retval false �������ɂ���
	 */
	inline bool IsOtherSide(const Vec3 &ep1, const Vec3 &ep2, const Vec3 &tri, const Vec3 &pos, double &ang)
	{
		Vec3 edge = ep1-ep2;	// ��G�b�W�x�N�g��
		Vec3 to_pos = pos-ep2;	// �ߖT�_�ւ̃x�N�g��
		Vec3 to_tri = tri-ep2;	// �O�p�`��1�̒��_�ւ̃x�N�g��

		// �ߖT�x�N�g���ƒ��_�x�N�g���̐����p
		ang = dot(Unit(cross(edge, to_pos)), Unit(cross(edge, to_tri)));

		if(ang < 0.0){	// �ߖT�_���P�̒��_�ɑ΂���Edge�̔��Α��ɑ���
			return true;
		}
		else{
			return false;
		}
	}

	/*!
	 * Test the point "alpha" of the way from P1 to P2 
	 * See if it is on a face of the cube   
	 * Consider only faces in "mask"				   
	 * @param 
	 * @return 
	 */
	static int CheckPoint(Vec3 p1, Vec3 p2, float alpha, long mask)
	{
		Vec3 plane_point;

		plane_point[0] = RX_LERP(p1[0], p2[0], (double)alpha);
		plane_point[1] = RX_LERP(p1[1], p2[1], (double)alpha);
		plane_point[2] = RX_LERP(p1[2], p2[2], (double)alpha);
		return ( FacePlane(plane_point) & mask);
	}

	/*!
	 * Compute intersection of P1 --> P2 line segment with face planes 
	 * Then test intersection point to see if it is on cube face	   
	 * Consider only face planes in "outcode_diff"					 
	 * Note: Zero bits in "outcode_diff" means face line is outside of 
	 * @param 
	 * @return 
	 */
	static bool CheckLine(Vec3 p1, Vec3 p2, int outcode_diff)
	{

	   if((0x01 & outcode_diff) != 0)
		  if(CheckPoint(p1, p2, (float)(( .5f-p1[0])/(p2[0]-p1[0])), 0x3e) == 0) return true;
	   if((0x02 & outcode_diff) != 0)
		  if(CheckPoint(p1, p2, (float)((-.5f-p1[0])/(p2[0]-p1[0])), 0x3d) == 0) return true;
	   if((0x04 & outcode_diff) != 0) 
		  if(CheckPoint(p1, p2, (float)(( .5f-p1[1])/(p2[1]-p1[1])), 0x3b) == 0) return true;
	   if((0x08 & outcode_diff) != 0) 
		  if(CheckPoint(p1, p2, (float)((-.5f-p1[1])/(p2[1]-p1[1])), 0x37) == 0) return true;
	   if((0x10 & outcode_diff) != 0) 
		  if(CheckPoint(p1, p2, (float)(( .5f-p1[2])/(p2[2]-p1[2])), 0x2f) == 0) return true;
	   if((0x20 & outcode_diff) != 0) 
		  if(CheckPoint(p1, p2, (float)((-.5f-p1[2])/(p2[2]-p1[2])), 0x1f) == 0) return true;

	   return false;
	}

}



#endif // _INTERSECTIONS_H_
