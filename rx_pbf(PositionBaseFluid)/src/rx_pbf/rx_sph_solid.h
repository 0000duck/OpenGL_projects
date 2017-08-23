/*!
  @file rx_sph_solid.h
	
  @brief SPH�p�ő̒�`
 
  @author Makoto Fujisawa
  @date 2008-12
*/

#ifndef _RX_SPH_SOLID_H_
#define _RX_SPH_SOLID_H_


//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------


#include "rx_sph_commons.h"
#include "rx_model.h"
#include "rx_nnsearch.h"	// �O���b�h�����ɂ��ߖT�T��
#include "rx_mesh.h"

// SPH�J�[�l��
#include "rx_kernel.h"


//-----------------------------------------------------------------------------
// ��`�E�萔
//-----------------------------------------------------------------------------

const int RX_DISPMAP_N = 128;


//-----------------------------------------------------------------------------
// MARK:rxCollisionInfo�N���X
//-----------------------------------------------------------------------------
class rxCollisionInfo
{
protected:
	Vec3 m_vContactPoint;	//!< �Փ˓_
	Vec3 m_vNormal;			//!< (�Փ˓_�ł�)�@��
	double m_fDepth;		//!< �߂荞�ݗ�

	Vec3 m_vVelocity;		//!< �Փ˓_�̑��x

public:
	//! �f�t�H���g�R���X�g���N�^
	rxCollisionInfo()
	  : m_vContactPoint(Vec3(0.0)), 
		m_vNormal(Vec3(0.0)), 
		m_fDepth(0.0), 
		m_vVelocity(Vec3(0.0))
	{
	}

	//! �R���X�g���N�^
	rxCollisionInfo(const Vec3 &contact_point, 
					const Vec3 &normal = Vec3(0.0), 
					const double &depth = 0.0, 
					const Vec3 &veloc = Vec3(0.0))
	  : m_vContactPoint(contact_point), 
		m_vNormal(normal), 
		m_fDepth(depth), 
		m_vVelocity(veloc)
	{
	}

	//! �f�X�g���N�^
	~rxCollisionInfo(){}

public:
	const Vec3& Contact() const { return m_vContactPoint; }
	Vec3& Contact(){ return m_vContactPoint; }

	const Vec3& Normal() const { return m_vNormal; }
	Vec3& Normal(){ return m_vNormal; }

	const double& Penetration() const { return m_fDepth; }
	double& Penetration(){ return m_fDepth; }

	const Vec3& Velocity() const { return m_vVelocity; }
	Vec3& Velocity(){ return m_vVelocity; }
};




//-----------------------------------------------------------------------------
// MARK:rxSolid : �ő̃I�u�W�F�N�g���N���X
//-----------------------------------------------------------------------------
class rxSolid
{
public:
	enum{
		RXS_SPHERE, 
		RXS_AABB, 
		RXS_OPEN_BOX, 
		RXS_POLYGON, 
		RXS_IMPLICIT, 
		RXS_OTHER = -1, 
	};

protected:
	Vec3 m_vMassCenter;	//!< �d�S���W
	Vec3 m_vVelocity;	//!< �ő̑��x
	rxMatrix4 m_matRot;	//!< �p��
	rxMatrix4 m_matRotInv;	//!< �p��

	int m_iName;

	RXREAL m_fOffset;

	bool m_bFix;		//!< �Œ�t���O
	bool m_bSP;			//!< �ő̃p�[�e�B�N�������t���O

	int m_iSgn;			//!< ��:-1, �I�u�W�F�N�g:1

public:
	rxSolid() : m_iName(RXS_OTHER)
	{
		m_bFix = true;
		m_vMassCenter = Vec3(0.0);
		m_vVelocity = Vec3(0.0);
		m_iSgn = 1;
		m_fOffset = (RXREAL)(0.0);
		m_bSP = true;

		m_matRot.MakeIdentity();
	}

	//
	// ���z�֐�
	//
	virtual bool GetDistance(const Vec3 &pos, rxCollisionInfo &col) = 0;	//!< �����֐��v�Z
	virtual bool GetDistanceR(const Vec3 &pos, const double &r, rxCollisionInfo &col) = 0;

	virtual bool GetDistance(const Vec3 &pos0, const Vec3 &pos1, rxCollisionInfo &col) = 0;	//!< �����֐��v�Z
	virtual bool GetDistanceR(const Vec3 &pos0, const Vec3 &pos1, const double &r, rxCollisionInfo &col) = 0;

	virtual bool GetCurvature(const Vec3 &pos, double &k) = 0;	//!< �����֐��̋ȗ��v�Z

	virtual void Draw(int drw) = 0;				//!< OpenGL�ł̕`��
	virtual void SetGLMatrix(void) = 0;							//!< OpenGL�ϊ��s��̓K�p

	virtual Vec3 GetMin(void) = 0;
	virtual Vec3 GetMax(void) = 0;


	//
	// �\�ʏ�Ƀp�[�e�B�N����z�u
	//
	static Vec4 GetImplicitG_s(void* ptr, double x, double y, double z);
	inline Vec4 GetImplicitG(Vec3 pos);
	static RXREAL GetImplicit_s(void* ptr, double x, double y, double z);
	inline RXREAL GetImplicit(Vec3 pos);

	int GenerateParticlesOnSurf(RXREAL rad, RXREAL **ppos);

	static bool GetDistance_s(Vec3 pos, rxCollisionInfo &col, void* x);

	//
	// �擾�E�ݒ�֐�
	//
	inline Vec3 CalLocalCoord(const Vec3 &pos);			//!< �O���[�o������ő̃��[�J���ւ̍��W�ϊ�
	inline Vec3 CalGlobalCoord(const Vec3 &pos);		//!< �ő̃��[�J������O���[�o���ւ̍��W�ϊ�

	inline Vec3 GetPosition(void);						//!< �ő̏d�S�ʒu�̎擾
	inline void SetPosition(const Vec3 &pos);			//!< �ő̏d�S�ʒu�̐ݒ�

	inline rxMatrix4 GetMatrix(void);					//!< ��]�s��̎擾
	inline void SetMatrix(const rxMatrix4 &mat);		//!< ��]�s��̐ݒ�
	inline void SetMatrix(double mat[16]);				//!< ��]�s��̐ݒ�

	inline Vec3 GetVelocityAtGrobal(const Vec3 &pos);	//!< �̍��W�l�̌ő̑��x�̎擾
	inline void SetVelocity(const Vec3 &vec);			//!< 

	inline bool GetFix(void) const { return m_bFix; }		//!< �Œ�t���O�̎擾
	inline void SetFix(bool fix){ m_bFix = fix; }			//!< �Œ�t���O�̐ݒ�

	inline bool& IsSolidParticles(void){ return m_bSP; }	//!< �ő̃p�[�e�B�N���t���O�̎擾

	inline bool RigidSimulation(const double &dt);		//!< ���̃V�~�����[�V����

	inline const int& Name() const { return m_iName; }
	inline int& Name(){ return m_iName; }
};



//-----------------------------------------------------------------------------
// MARK:rxSolidBox : ������
//-----------------------------------------------------------------------------
class rxSolidBox : public rxSolid
{
protected:
	Vec3 m_vMax, m_vMin;	//!< �ő���W�C�ŏ����W(���S����̑��Βl)
	Vec3 m_vC[2];

public:
	// �R���X�g���N�^
	rxSolidBox(Vec3 minp, Vec3 maxp, int sgn)
	{
		Vec3 sl  = 0.5*(maxp-minp);
		Vec3 ctr = 0.5*(maxp+minp);

		m_vMin = -sl;
		m_vMax =  sl;
		m_vMassCenter = ctr;

		m_vC[0] = m_vMin;
		m_vC[1] = m_vMax;

		m_iSgn = sgn;

		m_iName = RXS_AABB;
	}

	virtual Vec3 GetMin(void){ return m_vMassCenter+m_vMin; }
	virtual Vec3 GetMax(void){ return m_vMassCenter+m_vMax; }

	virtual bool GetDistance(const Vec3 &pos, rxCollisionInfo &col);
	virtual bool GetDistanceR(const Vec3 &pos, const double &r, rxCollisionInfo &col);

	virtual bool GetDistance(const Vec3 &pos0, const Vec3 &pos1, rxCollisionInfo &col){ return GetDistance(pos1, col); }
	virtual bool GetDistanceR(const Vec3 &pos0, const Vec3 &pos1, const double &r, rxCollisionInfo &col){ return GetDistanceR(pos1, r, col); }

	virtual bool GetCurvature(const Vec3 &pos, double &k);
	virtual void Draw(int drw = 4);
	virtual void SetGLMatrix(void);

protected:
	bool aabb_point_dist(const Vec3 &spos, const double &h, const int &sgn, const Vec3 &vMin, const Vec3 &vMax, 
						 double &d, Vec3 &n, int &np, int &plist, double pdist[6]);

};

//-----------------------------------------------------------------------------
// rxSolidOpenBox : ������(�J)
//-----------------------------------------------------------------------------
class rxSolidOpenBox : public rxSolid
{
protected:
	Vec3 m_vSLenIn, m_vSLenOut;

public:
	// �R���X�g���N�^
	rxSolidOpenBox(Vec3 ctr, Vec3 sl_in, Vec3 sl_out, int sgn)
	{
		m_vSLenIn  = sl_in;
		m_vSLenOut = sl_out;
		m_vMassCenter = ctr;

		RXCOUT << "SLenIn  " << m_vSLenIn << endl;
		RXCOUT << "SLenOut " << m_vSLenOut << endl;

		m_iSgn = sgn;

		m_iName = RXS_OPEN_BOX;
	}

	Vec3 GetInMin(void) const { return -m_vSLenIn; }
	Vec3 GetInMax(void) const { return  m_vSLenIn; }
	Vec3 GetOutMin(void) const { return -m_vSLenOut; }
	Vec3 GetOutMax(void) const { return  m_vSLenOut; }
	Vec3 GetInSideLength(void) const { return m_vSLenIn; }
	Vec3 GetOutSideLength(void) const { return m_vSLenOut; }

	virtual Vec3 GetMin(void){ return -m_vSLenOut; }
	virtual Vec3 GetMax(void){ return  m_vSLenOut; }

	virtual bool GetDistance(const Vec3 &pos, rxCollisionInfo &col);
	virtual bool GetDistanceR(const Vec3 &pos, const double &r, rxCollisionInfo &col);

	virtual bool GetDistance(const Vec3 &pos0, const Vec3 &pos1, rxCollisionInfo &col){ return GetDistance(pos1, col); }
	virtual bool GetDistanceR(const Vec3 &pos0, const Vec3 &pos1, const double &r, rxCollisionInfo &col){ return GetDistanceR(pos1, r, col); }

	virtual bool GetCurvature(const Vec3 &pos, double &k);
	virtual void Draw(int drw = 4);
	virtual void SetGLMatrix(void);
};



//-----------------------------------------------------------------------------
// rxSolidSphere : ��
//-----------------------------------------------------------------------------
class rxSolidSphere : public rxSolid
{
protected:
	double m_fRadius;		//!< ���a
	double m_fRadiusSqr;	//!< ���a�̎���

public:
	// �R���X�g���N�^
	rxSolidSphere(Vec3 ctr, double rad, int sgn)
		: m_fRadius(rad)
	{
		m_iSgn = sgn;
		m_vMassCenter = ctr;
		m_fRadiusSqr = rad*rad;

		m_iName = RXS_SPHERE;
	}

	Vec3 GetCenter(void) const { return m_vMassCenter; }
	double GetRadius(void) const { return m_fRadius; }

	virtual Vec3 GetMin(void){ return m_vMassCenter-Vec3(m_fRadius); }
	virtual Vec3 GetMax(void){ return m_vMassCenter+Vec3(m_fRadius); }

	virtual bool GetDistance(const Vec3 &pos, rxCollisionInfo &col);
	virtual bool GetDistanceR(const Vec3 &pos, const double &r, rxCollisionInfo &col);

	virtual bool GetDistance(const Vec3 &pos0, const Vec3 &pos1, rxCollisionInfo &col){ return GetDistance(pos1, col); }
	virtual bool GetDistanceR(const Vec3 &pos0, const Vec3 &pos1, const double &r, rxCollisionInfo &col){ return GetDistanceR(pos1, r, col); }

	virtual bool GetCurvature(const Vec3 &pos, double &k){ return false; }
	virtual void Draw(int drw = 4);
	virtual void SetGLMatrix(void);
};





//-----------------------------------------------------------------------------
// rxSolidPolygon : �|���S��
//-----------------------------------------------------------------------------
class rxSolidPolygon : public rxSolid
{
protected:
	Vec3 m_vMax, m_vMin;	//!< �ő���W�C�ŏ����W(���S����̑��Βl)

	string m_strFilename;

	rxPolygons m_Poly;
	RXREAL *m_hVrts;				//!< �ő̃|���S���̒��_
	int m_iNumVrts;					//!< �ő̃|���S���̒��_��
	int *m_hTris;					//!< �ő̃|���S��
	int m_iNumTris;					//!< �ő̃|���S���̐�

	double m_fMaxRad;
	rxNNGrid *m_pNNGrid;			//!< �����O���b�h�ɂ��ߖT�T��

public:
	// �R���X�g���N�^�ƃf�X�g���N�^
	rxSolidPolygon(const string &fn, Vec3 cen, Vec3 ext, Vec3 ang, double h, int aspect = 1);
	~rxSolidPolygon();

	virtual Vec3 GetMin(void){ return m_vMassCenter+m_vMin; }
	virtual Vec3 GetMax(void){ return m_vMassCenter+m_vMax; }

	virtual bool GetDistance(const Vec3 &pos, rxCollisionInfo &col);
	virtual bool GetDistanceR(const Vec3 &pos, const double &r, rxCollisionInfo &col);

	virtual bool GetDistance(const Vec3 &pos0, const Vec3 &pos1, rxCollisionInfo &col);
	virtual bool GetDistanceR(const Vec3 &pos0, const Vec3 &pos1, const double &r, rxCollisionInfo &col);

	virtual bool GetCurvature(const Vec3 &pos, double &k);
	virtual void Draw(int drw);
	virtual void SetGLMatrix(void);

	// �����Z��
	int  GetPolygonsInCell(int gi, int gj, int gk, set<int> &polys);
	bool IsPolygonsInCell(int gi, int gj, int gk);

protected:
	int getDistanceToPolygon(Vec3 x, int p, double &dist);

	int readFile(const string filename, rxPolygons &polys);
	void readOBJ(const string filename, rxPolygons &polys);

	bool fitVertices(const Vec3 &ctr, const Vec3 &sl, vector<Vec3> &vec_set, int aspect = 1);

	int intersectSegmentTriangle(Vec3 P0, Vec3 P1, Vec3 V0, Vec3 V1, Vec3 V2, Vec3 &I, Vec3 &n);

};




//-----------------------------------------------------------------------------
// MARK:rxSolid�N���X�̎���
//-----------------------------------------------------------------------------
/*!
 * �O���[�o������ő̃��[�J���ւ̍��W�ϊ�
 * @param[in] x,y,z �O���[�o�����W�n�ł̈ʒu
 * @return �ő̃��[�J�����W�n�ł̈ʒu
 */
inline Vec3 rxSolid::CalLocalCoord(const Vec3 &pos)
{
	// ���̍��W����ő̍��W�ւƕϊ�
	Vec3 rpos;
	rpos = pos-m_vMassCenter;
	//m_matRot.multMatrixVec(rpos);
	rpos[0] = rpos[0]*m_matRot(0,0)+rpos[1]*m_matRot(1,0)+rpos[2]*m_matRot(2,0);
	rpos[1] = rpos[0]*m_matRot(0,1)+rpos[1]*m_matRot(1,1)+rpos[2]*m_matRot(2,1);
	rpos[2] = rpos[0]*m_matRot(0,2)+rpos[1]*m_matRot(1,2)+rpos[2]*m_matRot(2,2);
	return rpos;
}
/*!
 * �ő̃��[�J������O���[�o���ւ̍��W�ϊ�
 * @param[in] x,y,z �ő̃��[�J�����W�n�ł̈ʒu
 * @return �O���[�o�����W�n�ł̈ʒu
 */
inline Vec3 rxSolid::CalGlobalCoord(const Vec3 &pos)
{
	// �ő̍��W���痬�̍��W�ւƕϊ�
	Vec3 fpos = pos;
	//m_matRotInv.multMatrixVec(pos, fpos);
	fpos[0] = pos[0]*m_matRotInv(0,0)+pos[1]*m_matRotInv(1,0)+pos[2]*m_matRotInv(2,0);
	fpos[1] = pos[0]*m_matRotInv(0,1)+pos[1]*m_matRotInv(1,1)+pos[2]*m_matRotInv(2,1);
	fpos[2] = pos[0]*m_matRotInv(0,2)+pos[1]*m_matRotInv(1,2)+pos[2]*m_matRotInv(2,2);
	fpos = fpos+m_vMassCenter;
	return fpos;
}

/*!
 * �ő̏d�S�ʒu�̎擾
 * @return �ő̏d�S���W(���̍��W�n)
 */
inline Vec3 rxSolid::GetPosition(void)
{
	return m_vMassCenter;
}

/*!
 * �ő̏d�S�ʒu�̐ݒ�
 * @param[in] pos �ő̏d�S���W(���̍��W�n)
 */
void rxSolid::SetPosition(const Vec3 &pos)
{
	m_vMassCenter = pos;
}

/*!
 * �ő̏d�S�ʒu�̎擾
 * @return �ő̏d�S���W(���̍��W�n)
 */
inline rxMatrix4 rxSolid::GetMatrix(void)
{
	return m_matRot;
}


inline rxMatrix4 CalInverseMatrix(const rxMatrix4 &mat)
{
	real d = mat(0, 0)*mat(1, 1)*mat(2, 2)-mat(0, 0)*mat(2, 1)*mat(1, 2)+ 
			 mat(1, 0)*mat(2, 1)*mat(0, 2)-mat(1, 0)*mat(0, 1)*mat(2, 2)+ 
			 mat(2, 0)*mat(0, 1)*mat(1, 2)-mat(2, 0)*mat(1, 1)*mat(0, 2);

	if(d == 0) d = 1;

	return rxMatrix4( (mat(1, 1)*mat(2, 2)-mat(1, 2)*mat(2, 1))/d,
					 -(mat(0, 1)*mat(2, 2)-mat(0, 2)*mat(2, 1))/d,
					  (mat(0, 1)*mat(1, 2)-mat(0, 2)*mat(1, 1))/d,
					  0.0, 
					 -(mat(1, 0)*mat(2, 2)-mat(1, 2)*mat(2, 0))/d,
					  (mat(0, 0)*mat(2, 2)-mat(0, 2)*mat(2, 0))/d,
					 -(mat(0, 0)*mat(1, 2)-mat(0, 2)*mat(1, 0))/d,
					  0.0, 
					  (mat(1, 0)*mat(2, 1)-mat(1, 1)*mat(2, 0))/d,
					 -(mat(0, 0)*mat(2, 1)-mat(0, 1)*mat(2, 0))/d,
					  (mat(0, 0)*mat(1, 1)-mat(0, 1)*mat(1, 0))/d,
					  0.0, 
					  0.0, 0.0, 0.0, 1.0);
}


/*!
 * �ő̏d�S�ʒu�̐ݒ�
 * @param[in] pos �ő̏d�S���W(���̍��W�n)
 */
void rxSolid::SetMatrix(const rxMatrix4 &mat)
{
	//m_matRot = mat;
	for(int i = 0; i < 3; ++i){
		for(int j = 0; j < 3; ++j){
			m_matRot(i, j) = mat(i, j);
		}
	}

	//m_matRotInv = m_matRot.Inverse();
	m_matRotInv = CalInverseMatrix(m_matRot);
}

/*!
 * �ő̏d�S�ʒu�̐ݒ�
 * @param[in] pos �ő̏d�S���W(���̍��W�n)
 */
void rxSolid::SetMatrix(double mat[16])
{
	//m_matRot = mat;
	for(int i = 0; i < 4; ++i){
		for(int j = 0; j < 4; ++j){
			m_matRot(i, j) = mat[i+j*4];
		}
	}

	//m_matRotInv = m_matRot.Inverse();
	m_matRotInv = CalInverseMatrix(m_matRot);
}


/*!
 * �O���[�o�����W�l�ł̌ő̑��x�̎擾
 * @param[in] pos �O���[�o�����W�l
 * @return �ő̑��x
 */
inline Vec3 rxSolid::GetVelocityAtGrobal(const Vec3 &pos)
{
	return m_vVelocity;
}

/*!
 * �ő̑��x���Z�b�g
 * @param[in] vec �d�S���x
 */
inline void rxSolid::SetVelocity(const Vec3 &vec)
{
	m_vVelocity = vec;
}


/*!
 * ���̃V�~�����[�V����(fix=true�̎�)
 * @param[in] dt �^�C���X�e�b�v��
 */
inline bool rxSolid::RigidSimulation(const double &dt)
{
	m_vMassCenter += dt*m_vVelocity;
	return true;
}


	
	
//-----------------------------------------------------------------------------
// MARK:���̑��֐�
//-----------------------------------------------------------------------------
inline bool GetImplicitPlane(const Vec3 &pos, double &d, Vec3 &n, Vec3 &v, const Vec3 &pn, const Vec3 &pq)
{
	d = dot(pq-pos, pn);
	n = pn;
	v = Vec3(0.0);

	return true;
}


/*!
 * �����֐�����ȗ����v�Z
 * @param[in] pos �v�Z�_
 * @param[out] k �ȗ�
 * @param[in] fpDist �����֐�
 */
inline bool CalCurvature(const Vec3 &pos, double &k, bool (fpDist)(Vec3, rxCollisionInfo&, void*), void* fp = 0)
{
	k = 0.0;

	double h = 0.005;
	double x0, y0, z0;
	double p[3][3][3];
	rxCollisionInfo col;

	x0 = pos[0]-0.5*h;
	y0 = pos[1]-0.5*h;
	z0 = pos[2]-0.5*h;

//	fpDist(Vec3(x0-h, y0-h, z0-h), col, fp); p[0][0][0] = col.Penetration();
	fpDist(Vec3(x0-h, y0-h, z0  ), col, fp); p[0][0][1] = col.Penetration();
//	fpDist(Vec3(x0-h, y0-h, z0+h), col, fp); p[0][0][2] = col.Penetration();
	fpDist(Vec3(x0-h, y0  , z0-h), col, fp); p[0][1][0] = col.Penetration();
	fpDist(Vec3(x0-h, y0  , z0  ), col, fp); p[0][1][1] = col.Penetration();
	fpDist(Vec3(x0-h, y0  , z0+h), col, fp); p[0][1][2] = col.Penetration();
//	fpDist(Vec3(x0-h, y0+h, z0-h), col, fp); p[0][2][0] = col.Penetration();
	fpDist(Vec3(x0-h, y0+h, z0  ), col, fp); p[0][2][1] = col.Penetration();
//	fpDist(Vec3(x0-h, y0+h, z0+h), col, fp); p[0][2][2] = col.Penetration();

	fpDist(Vec3(x0  , y0-h, z0-h), col, fp); p[1][0][0] = col.Penetration();
	fpDist(Vec3(x0  , y0-h, z0  ), col, fp); p[1][0][1] = col.Penetration();
	fpDist(Vec3(x0  , y0-h, z0+h), col, fp); p[1][0][2] = col.Penetration();
	fpDist(Vec3(x0  , y0  , z0-h), col, fp); p[1][1][0] = col.Penetration();
	fpDist(Vec3(x0  , y0  , z0  ), col, fp); p[1][1][1] = col.Penetration();
	fpDist(Vec3(x0  , y0  , z0+h), col, fp); p[1][1][2] = col.Penetration();
	fpDist(Vec3(x0  , y0+h, z0-h), col, fp); p[1][2][0] = col.Penetration();
	fpDist(Vec3(x0  , y0+h, z0  ), col, fp); p[1][2][1] = col.Penetration();
	fpDist(Vec3(x0  , y0+h, z0+h), col, fp); p[1][2][2] = col.Penetration();

//	fpDist(Vec3(x0+h, y0-h, z0-h), col, fp); p[2][0][0] = col.Penetration();
	fpDist(Vec3(x0+h, y0-h, z0  ), col, fp); p[2][0][1] = col.Penetration();
//	fpDist(Vec3(x0+h, y0-h, z0+h), col, fp); p[2][0][2] = col.Penetration();
	fpDist(Vec3(x0+h, y0  , z0-h), col, fp); p[2][1][0] = col.Penetration();
	fpDist(Vec3(x0+h, y0  , z0  ), col, fp); p[2][1][1] = col.Penetration();
	fpDist(Vec3(x0+h, y0  , z0+h), col, fp); p[2][1][2] = col.Penetration();
//	fpDist(Vec3(x0+h, y0+h, z0-h), col, fp); p[2][2][0] = col.Penetration();
	fpDist(Vec3(x0+h, y0+h, z0  ), col, fp); p[2][2][1] = col.Penetration();
//	fpDist(Vec3(x0+h, y0+h, z0+h), col, fp); p[2][2][2] = col.Penetration();

	double px, py, pz, pxx, pyy, pzz, pxy, pyz, pxz, np;
	px = (p[2][1][1]-p[0][1][1])/(2.0*h);
	py = (p[1][2][1]-p[1][0][1])/(2.0*h);
	pz = (p[1][1][2]-p[1][1][0])/(2.0*h);

	pxx = (p[2][1][1]-2.0*p[1][1][1]+p[0][1][1])/(h*h);
	pyy = (p[1][2][1]-2.0*p[1][1][1]+p[1][0][1])/(h*h);
	pzz = (p[1][1][2]-2.0*p[1][1][1]+p[1][1][0])/(h*h);

	pxy = (p[0][0][1]+p[2][2][1]-p[0][2][1]-p[2][0][1])/(4.0*h*h);
	pxz = (p[0][1][0]+p[2][1][2]-p[0][1][2]-p[2][1][0])/(4.0*h*h);
	pyz = (p[1][0][0]+p[1][2][2]-p[1][0][2]-p[1][2][0])/(4.0*h*h);

	np = px*px+py*py+pz*pz;
	if(np > RX_FEQ_EPS){
		np = sqrt(np);

		// �ȗ��̌v�Z
		k = (px*px*pyy-2.0*px*py*pxy+py*py*pxx+px*px*pzz-2.0*px*pz*pxz+pz*pz*pxx+py*py*pzz-2.0*py*pz*pyz+pz*pz*pyy)/(np*np*np);
	}

	k = -k;

	return true;
}

/*!
 * Poly6�J�[�l���̒l���v�Z
 * @param[in] r �J�[�l�����S�܂ł̋���
 * @param[in] h �L�����a
 * @param[in] ptr �֐��Ăяo���|�C���^
 * @return �J�[�l���֐��l
 */
static inline double CalKernelPoly6(double r, double h, void *ptr = 0)
{
	static double a = 0.0;
	if(a == 0.0) a = KernelCoefPoly6(h, 3, 1);	
	return KernelPoly6(r, h, a);
}

/*!
 * Poly6�J�[�l���̒l���v�Z(�ő�l��1�ɂȂ�悤�ɐ��K��)
 * @param[in] r �J�[�l�����S�܂ł̋���
 * @param[in] h �L�����a
 * @param[in] ptr �֐��Ăяo���|�C���^
 * @return �J�[�l���֐��l
 */
static inline double CalKernelPoly6r(double r, double h, void *ptr = 0)
{
	static double a = 0.0;
	static double b = 0.0;
	if(a == 0.0) a = KernelCoefPoly6(h, 3, 1);	
	if(b == 0.0) b = KernelPoly6(0.0, h, a);
	return KernelPoly6(r, h, a)/b;
}


/*!
 * ����(���C,������)�Ƌ��̌�������
 * @param[in] p,d ���C�̌��_�ƕ���
 * @param[in] c,r ���̒��S�Ɣ��a
 * @param[out] t1,t2 p�����_�܂ł̋���
 * @return ��_��
 */
inline int ray_sphere(const Vec3 &p, const Vec3 &d, const Vec3 &sc, const double r, double &t1, double &t2)
{
	Vec3 q = p-sc;	// �����S���W�n�ł̌������_���W

	double a = norm2(d);
	double b = 2*dot(q, d);
	double c = norm2(q)-r*r;

	// ���ʎ�
	double D = b*b-4*a*c;

	if(D < 0.0){ // �����Ȃ�
		return 0;
	}
	else if(D < RX_FEQ_EPS){ // ��_��1
		t1 = -b/(2*a);
		t2 = -1;
		return 1;
	}
	else{ // ��_��2
		double sqrtD = sqrt(D);
		t1 = (-b-sqrtD)/(2*a);
		t2 = (-b+sqrtD)/(2*a);
		return 2;
	}

}

/*!
 * �O�p�`�Ƌ��̌�������
 * @param[in] v0,v1,v2	�O�p�`�̒��_
 * @param[in] n			�O�p�`�̖@��
 * @param[in] p			�ŋߖT�_
 * @return 
 */
inline bool triangle_sphere(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2, const Vec3 &n, 
							const Vec3 &c, const double &r, double &dist, Vec3 &ipoint)
{
	// �|���S�����܂ޕ��ʂƋ����S�̋���
	double d = dot(v0, n);
	double l = dot(n, c)-d;

	dist = l;
	if(l > r) return false;

	// ���ʂƂ̍ŋߖT�_���W
	Vec3 p = c-l*n;

	// �ߖT�_���O�p�`�����ǂ����̔���
	Vec3 n1 = cross((v0-c), (v1-c));
	Vec3 n2 = cross((v1-c), (v2-c));
	Vec3 n3 = cross((v2-c), (v0-c));

	ipoint = p;
	dist = l;
	if(dot(n1, n2) > 0 && dot(n2, n3) > 0){		// �O�p�`��
		return true;
	}
	else{		// �O�p�`�O
		// �O�p�`�̊e�G�b�W�Ƌ��̏Փ˔���
		for(int e = 0; e < 3; ++e){
			Vec3 va0 = (e == 0 ? v0 : (e == 1 ? v1 : v2));
			Vec3 va1 = (e == 0 ? v1 : (e == 1 ? v2 : v0));

			double t1, t2;
			int n = ray_sphere(va0, Unit(va1-va0), c, r, t1, t2);

			if(n){
				double le2 = norm2(va1-va0);
				if((t1 >= 0.0 && t1*t1 < le2) || (t2 >= 0.0 && t2*t2 < le2)){
					return true;
				}
			}
		}
		return false;
	}
}

/*!
 * ����(���܂ޒ���)�Ɠ_�̋���
 * @param[in] v0,v1 �����̗��[�_���W
 * @param[in] p �_�̍��W
 * @return ����
 */
inline double segment_point_dist(const Vec3 &v0, const Vec3 &v1, const Vec3 &p)
{
	Vec3 v = Unit(v1-v0);
	Vec3 vp = p-v0;
	Vec3 vh = dot(vp, v)*v;
	return norm(vp-vh);
}

/*!
 * �����Ƌ��̌�������
 * @param[in] s0,s1	�����̒[�_
 * @param[in] sc,r   ���̒��S���W�Ɣ��a
 * @param[out] d2 �����Ƃ̋����̓��
 * @return ���������true
 */
inline bool segment_sphere(const Vec3 &s0, const Vec3 &s1, const Vec3 &sc, const double &r, double &d2)
{
	Vec3 v = s1-s0;
	Vec3 c = sc-s0;

	double vc = dot(v, c);
	if(vc < 0){		// ���̒��S�������̎n�_s0�̊O�ɂ���
		d2 = norm2(c);
		return (d2 < r*r);	// �����S�Ǝn�_s0�̋����Ō�������
	}
	else{
		double v2 = norm2(v);
		if(vc > v2){	// ���̒��S�������̏I�_s1�̊O�ɂ���
			d2 = norm2(s1-sc);
			return (d2 < r*r);	// �����S�ƏI�_s1�̋����Ō�������
		}
		else{			// ����s0��s1�̊Ԃɂ���
			d2 = norm2((vc*v)/norm2(v)-c);
			return (d2 < r*r);	// �����Ƌ����S�̋����Ō�������
		}
	}

	return false;
}





#endif	// _RX_SPH_SOLID_H_
