/*! 
  @file rx_sampler.h
	
  @brief �T���v���_����
 
  @author Makoto Fujisawa
  @date 2013-06
*/

#ifndef _RX_SAMPLER_
#define _RX_SAMPLER_


//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
// STL
#include <vector>
#include <list>

#include "rx_utility.h"

using namespace std;


//-----------------------------------------------------------------------------
// ���̃T���v���̐e�N���X
//-----------------------------------------------------------------------------
class rxSampler
{
protected:
	Vec2 m_v2MinPos, m_v2MaxPos;	//!< �T���v�����O�͈�
	Vec2 m_v2Dim;					//!< �T���v�����O�͈͂̑傫��
	int m_iNumMax;					//!< �ő�T���v�����O��

	double m_fDmin;					//!< �T���v�����O�_�Ԃ̍ŏ�����
	int m_iNumAround;				//!< �_���͂̃T���v�����O��

public:
	/*!
	 * �R���X�g���N�^
	 * @param[in] minp,maxp �����͈�
	 * @param[in] num_max �ő�_��
	 */
	rxSampler(Vec2 minp, Vec2 maxp, int num_max)
	{
		m_v2MinPos = minp;
		m_v2MaxPos = maxp;
		m_v2Dim = maxp-minp;
		m_iNumMax = num_max;
	}

	//! �f�X�g���N�^
	virtual ~rxSampler()
	{
	}
	
	//! �_�̃T���v�����O
	virtual void Generate(vector<Vec2> &points) = 0;

protected:
	/*!
	 * ���W�l���炻�̓_���܂܂��Z���ʒu��Ԃ�
	 * @param[in] p ���W�l
	 * @param[out] i,j �Z���ʒu
	 * @param[in] h �O���b�h��
	 */
	void calGridPos(Vec2 p, int &i, int &j, double h)
	{
		Vec2 dp = (p-m_v2MinPos)/h;
		i = (int)(dp[0]);
		j = (int)(dp[1]);
	}

	/*!
	 * ���S�_�����苗�����̃����_���_�𐶐�
	 * @param[in] cen ���S�_���W
	 * @param[in] d_min �ŏ�����(2*d_min���ő�)
	 * @return �����_���_
	 */
	Vec2 genRandomAroundPoint(Vec2 cen, double d_min)
	{
		// �ǉ�����_�܂ł̋���(���a)�Ɗp�x�𗐐��Őݒ�
		double rad = d_min*(1.0+RXFunc::Frand());
		double ang = 2*RX_PI*(RXFunc::Frand());

		// cen���狗��[d_min, 2*d_min]���̓_
		Vec2 new_p = cen+rad*Vec2(sin(ang), cos(ang));

		return new_p;
	}

	inline int IDX(int i, int j, int nx) const { return i+j*nx; }
};


//-----------------------------------------------------------------------------
// �����_���T���v��
//  - �������g�����T���v���_�̐���
//-----------------------------------------------------------------------------
class rxRandomSampler : public rxSampler
{
protected:
	rxRandomSampler() : rxSampler(Vec2(0.0), Vec2(1.0), 100){}

public:
	/*!
	 * �R���X�g���N�^
	 * @param[in] minp,maxp �����͈�
	 * @param[in] num_max �ő�_��
	 */
	rxRandomSampler(Vec2 minp, Vec2 maxp, int num_max) : rxSampler(minp, maxp, num_max)
	{
	}

	//! �f�X�g���N�^
	virtual ~rxRandomSampler(){}

	/*!
	 * �T���v�����O�_�̐���
	 * @param[out] points �T���v�����O���ꂽ�_
	 */
	virtual void Generate(vector<Vec2> &points)
	{
		points.resize(m_iNumMax);
		for(int i = 0; i < m_iNumMax; ++i){
			points[i] = RXFunc::Rand(m_v2MaxPos, m_v2MinPos);
		}
	}
};


//-----------------------------------------------------------------------------
// Uniform Poisson Disk Sampling
//  - R. Bridson, "Fast Poisson Disk Sampling in Arbitrary Dimensions", SIGGRAPH2007 sketches. 
//  - http://devmag.org.za/2009/05/03/poisson-disk-sampling/
//-----------------------------------------------------------------------------
class rxUniformPoissonDiskSampler : public rxSampler
{
	// �T���v���_���i�[����O���b�h�Z�����
	struct rxPointGrid
	{
		Vec2 point;		//!< �i�[���ꂽ�_���W
		int num;		//!< �i�[���ꂽ�_�̐�(0 or 1)
		rxPointGrid() : num(0){}
	};

	double m_fH;					//!< �O���b�h��
	int m_iNx, m_iNy;				//!< �O���b�h��

	vector<rxPointGrid> m_vGrid;	//!< �_���i�[����O���b�h

protected:
	rxUniformPoissonDiskSampler() : rxSampler(Vec2(0.0), Vec2(1.0), 100){}

public:
	/*!
	 * �R���X�g���N�^
	 * @param[in] minp,maxp �����͈�
	 * @param[in] num_max �ő�_��
	 * @param[in] num_around �_���͂̃T���v�����O��
	 * @param[in] d_min �T���v�����O�_�Ԃ̍ŏ�����
	 */
	rxUniformPoissonDiskSampler(Vec2 minp, Vec2 maxp, int num_max, int num_around, double d_min)
		: rxSampler(minp, maxp, num_max)
	{
		// �_���i�[����O���b�h
		// �i�q����r/��d�Ƃ��邱�Ƃ�1�_/�O���b�h�ɂȂ�悤�ɂ���
		m_fH = d_min/RX_ROOT2;
		m_iNx = (int)(m_v2Dim[0]/m_fH)+1;
		m_iNy = (int)(m_v2Dim[1]/m_fH)+1;

		m_vGrid.resize(m_iNx*m_iNy);

		m_fDmin = d_min;
		m_iNumAround = num_around;
	}

	//! �f�X�g���N�^
	virtual ~rxUniformPoissonDiskSampler(){}

protected:
	/*!
	 * �ŏ��̃T���v�����O�_�̒ǉ�
	 * @param[in] grid �_���i�[����O���b�h
	 * @param[out] active �T���v�����O�ʒu����p��_
	 * @param[out] point �T���v�����O�_
	 */	
	void addFirstPoint(list<Vec2> &active, vector<Vec2> &point)
	{
		// �����_���Ȉʒu�ɓ_�𐶐�
		Vec2 p = RXFunc::Rand(m_v2MaxPos, m_v2MinPos);

		// �_���܂ރO���b�h�ʒu
		int i, j;
		calGridPos(p, i, j, m_fH);
		int g = IDX(i, j, m_iNx);
		m_vGrid[g].point = p;
		m_vGrid[g].num = 1;

		active.push_back(p);
		point.push_back(p);
	}


	/*!
	 * �T���v�����O�_�̒ǉ�
	 * @param[in] grid �_���i�[����O���b�h
	 * @param[out] active �T���v�����O�ʒu����p��_
	 * @param[out] point �T���v�����O�_
	 */	
	void addPoint(list<Vec2> &active, vector<Vec2> &point, Vec2 p)
	{
		// �_���܂ރO���b�h�ʒu
		int i, j;
		calGridPos(p, i, j, m_fH);
		int g = IDX(i, j, m_iNx);
		m_vGrid[g].point = p;
		m_vGrid[g].num = 1;

		active.push_back(p);
		//point.push_back(p);
	}


	/*!
	 * �T���v�����O�_�̒ǉ�
	 * @param[in] grid �_���i�[����O���b�h
	 * @param[out] active �T���v�����O�ʒu����p��_
	 * @param[out] point �T���v�����O�_
	 */	
	bool addNextPoint(list<Vec2> &active, vector<Vec2> &point, Vec2 p)
	{
		bool found = false;

		Vec2 q = genRandomAroundPoint(p, m_fDmin);

		if(RXFunc::InRange(q, m_v2MinPos, m_v2MaxPos)){
			int x, y;	// q���܂܂��O���b�h�Z���ʒu
			calGridPos(q, x, y, m_fH);

			bool close = false;

			// ���łɒǉ����ꂽ�_�̎��͂ɒ����I�ɓ_��ǉ����Ă���
			// ���͂̃O���b�h��T�����āC�߂�����_(������d_min��菬����)���Ȃ������ׂ�
			for(int i = RX_MAX(0, x-2); i < RX_MIN(m_iNx, x+3) && !close; ++i){
				for(int j = RX_MAX(0, y-2); j < RX_MIN(m_iNy, y+3) && !close; ++j){
					int g = IDX(i, j, m_iNx);
					if(m_vGrid[g].num){	// �O���b�h�ɂ��łɓ_���܂܂�Ă���ꍇ
						double dist = norm(m_vGrid[g].point-q);
						if(dist < m_fDmin){
							close = true;
						}
					}
				}
			}

			// �߂�����_���Ȃ���ΐV�����_�Ƃ��Ēǉ�
			if(!close){
				found = true;
				active.push_back(q);
				point.push_back(q);
				int g = IDX(x, y, m_iNx);
				m_vGrid[g].point = q;
				m_vGrid[g].num = 1;
			}

		}

		return found;
	}

public:
	/*!
	 * �T���v�����O
	 * @param[out] points �T���v�����O���ꂽ�_
	 */
	virtual void Generate(vector<Vec2> &points)
	{
		// �_���i�[����O���b�h�̏�����
		int gnum = m_iNx*m_iNy;
		for(int k = 0; k < gnum; ++k){
			m_vGrid[k].point = Vec2(0.0);
			m_vGrid[k].num = 0;
		}

		list<Vec2> active;
		list<Vec2>::const_iterator itr;

		if(points.empty()){
			// �ŏ��̓_�̒ǉ�(�����_���Ȉʒu)
			addFirstPoint(active, points);
		}
		else{
			// ���O�ɒǉ����ꂽ�_
			for(vector<Vec2>::iterator i = points.begin(); i != points.end(); ++i){
				addPoint(active, points, *i);
			}
		}

		// ����ȏ�_���ǉ��ł��Ȃ� or �ő�_�� �܂ŃT���v�����O
		while((!active.empty()) && ((int)points.size() <= m_iNumMax)){
			// �A�N�e�B�u���X�g���烉���_����1�_�����o��
			int idx = RXFunc::Nrand(active.size());
			itr = active.begin();
			for(int i = 0; i < idx; ++i) itr++;
			Vec2 p = *(itr);

			// �_p����[d_min,2*d_min]�̈ʒu�ł����̓_�ɋ߂����Ȃ��ʒu�ɐV�����_��ǉ�
			bool found = false;
			for(int i = 0; i < m_iNumAround; ++i){
				found |= addNextPoint(active, points, p);
			}

			// �_p�̎���ɂ���ȏ�_��ǉ��ł��Ȃ��ꍇ��active���X�g����폜
			if(!found){
				active.erase(itr);
			}
		}
	}
};


//-----------------------------------------------------------------------------
// Poisson Disk Sampling with a distribution function
//  - R. Bridson, "Fast Poisson Disk Sampling in Arbitrary Dimensions", SIGGRAPH2007 sketches. 
//  - http://devmag.org.za/2009/05/03/poisson-disk-sampling/
//  - ���z�֐��̒l�ɂ�蕔���I�ɓ_�̖��x��ς���
//-----------------------------------------------------------------------------
class rxPoissonDiskSampler : public rxSampler
{
	// �T���v���_���i�[����O���b�h�Z�����
	struct rxPointGrid
	{
		vector<Vec2> points;		//!< �i�[���ꂽ�_���W
		int num;		//!< �i�[���ꂽ�_�̐�(0 or 1)
		rxPointGrid() : num(0){}
	};

	double m_fH;					//!< �O���b�h��
	int m_iNx, m_iNy;				//!< �O���b�h��

	vector<rxPointGrid> m_vGrid;	//!< �_���i�[����O���b�h

	double (*m_fpDist)(Vec2);		//!< ���z�֐�

protected:
	rxPoissonDiskSampler() : rxSampler(Vec2(0.0), Vec2(1.0), 100){}

public:
	/*!
	 * �R���X�g���N�^
	 * @param[in] minp,maxp �����͈�
	 * @param[in] num_max �ő�_��
	 * @param[in] num_around �_���͂̃T���v�����O��
	 * @param[in] d_min �T���v�����O�_�Ԃ̍ŏ�����
	 * @param[in] dist ���z�֐�
	 */
	rxPoissonDiskSampler(Vec2 minp, Vec2 maxp, int num_max, int num_around, double d_min, double (*dist)(Vec2))
		: rxSampler(minp, maxp, num_max)
	{
		// �_���i�[����O���b�h
		// �i�q����r/��d�Ƃ��邱�Ƃ�1�_/�O���b�h�ɂȂ�悤�ɂ���
		m_fH = d_min/RX_ROOT2;
		m_iNx = (int)(m_v2Dim[0]/m_fH)+1;
		m_iNy = (int)(m_v2Dim[1]/m_fH)+1;

		m_vGrid.resize(m_iNx*m_iNy);

		m_fDmin = d_min;
		m_iNumAround = num_around;

		m_fpDist = dist;
	}

	//! �f�X�g���N�^
	virtual ~rxPoissonDiskSampler(){}

protected:
	/*!
	 * �ŏ��̃T���v�����O�_�̒ǉ�
	 * @param[in] grid �_���i�[����O���b�h
	 * @param[out] active �T���v�����O�ʒu����p��_
	 * @param[out] point �T���v�����O�_
	 */	
	void addFirstPoint(list<Vec2> &active, vector<Vec2> &point)
	{
		// �����_���Ȉʒu�ɓ_�𐶐�
		Vec2 p = RXFunc::Rand(m_v2MaxPos, m_v2MinPos);

		// �_���܂ރO���b�h�ʒu
		int i, j;
		calGridPos(p, i, j, m_fH);
		int g = IDX(i, j, m_iNx);
		m_vGrid[g].points.push_back(p);
		m_vGrid[g].num++;

		active.push_back(p);
		point.push_back(p);
	}

	/*!
	 * �T���v�����O�_�̒ǉ�
	 * @param[in] grid �_���i�[����O���b�h
	 * @param[out] active �T���v�����O�ʒu����p��_
	 * @param[out] point �T���v�����O�_
	 */	
	bool addNextPoint(list<Vec2> &active, vector<Vec2> &point, Vec2 p)
	{
		bool found = false;

		double fraction = m_fpDist(p);
		Vec2 q = genRandomAroundPoint(p, m_fDmin);

		if(RXFunc::InRange(q, m_v2MinPos, m_v2MaxPos)){
			int x, y;	// q���܂܂��O���b�h�Z���ʒu
			calGridPos(q, x, y, m_fH);

			bool close = false;

			// ���łɒǉ����ꂽ�_�̎��͂ɒ����I�ɓ_��ǉ����Ă���
			// ���͂̃O���b�h��T�����āC�߂�����_(������d_min��菬����)���Ȃ������ׂ�
			for(int i = RX_MAX(0, x-2); i < RX_MIN(m_iNx, x+3) && !close; ++i){
				for(int j = RX_MAX(0, y-2); j < RX_MIN(m_iNy, y+3) && !close; ++j){
					int g = IDX(i, j, m_iNx);

					// �O���b�h���̓_�𒲂ׂĂ���
					for(int l = 0; l < m_vGrid[g].num; ++l){
						double dist = norm(m_vGrid[g].points[l]-q);
						if(dist < m_fDmin*fraction){
							close = true;
						}
					}
				}
			}

			// �߂�����_���Ȃ���ΐV�����_�Ƃ��Ēǉ�
			if(!close){
				found = true;
				active.push_back(q);
				point.push_back(q);
				int g = IDX(x, y, m_iNx);
				m_vGrid[g].points.push_back(q);
				m_vGrid[g].num++;
			}

		}

		return found;
	}

public:
	/*!
	 * �T���v�����O
	 * @param[out] points �T���v�����O���ꂽ�_
	 */
	virtual void Generate(vector<Vec2> &points)
	{
		// �_���i�[����O���b�h�̏�����
		int gnum = m_iNx*m_iNy;
		for(int k = 0; k < gnum; ++k){
			m_vGrid[k].points.clear();
			m_vGrid[k].num = 0;
		}

		list<Vec2> active;
		list<Vec2>::const_iterator itr;

		// �ŏ��̓_�̒ǉ�(�����_���Ȉʒu)
		addFirstPoint(active, points);

		// ����ȏ�_���ǉ��ł��Ȃ� or �ő�_�� �܂ŃT���v�����O
		while((!active.empty()) && ((int)points.size() <= m_iNumMax)){
			// �A�N�e�B�u���X�g���烉���_����1�_�����o��
			int idx = RXFunc::Nrand(active.size());
			itr = active.begin();
			for(int i = 0; i < idx; ++i) itr++;
			Vec2 p = *(itr);

			// �_p����[d_min,2*d_min]�̈ʒu�ł����̓_�ɋ߂����Ȃ��ʒu�ɐV�����_��ǉ�
			bool found = false;
			for(int i = 0; i < m_iNumAround; ++i){
				found |= addNextPoint(active, points, p);
			}

			// �_p�̎���ɂ���ȏ�_��ǉ��ł��Ȃ��ꍇ��active���X�g����폜
			if(!found){
				active.erase(itr);
			}
		}
	}
};

#endif // _RX_SAMPLER_