/*! 
  @file rx_delaunay.h
	
  @brief �h���l�[�O�p�`����
 
  @author Makoto Fujisawa
  @date 2014-07
*/

#ifndef _RX_DELAUNAY_H_
#define _RX_DELAUNAY_H_


//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
// C�W��
#include <iostream>

// STL
#include <vector>

// Vec2,Vec3�Ȃ�
#include "rx_utility.h"


//-----------------------------------------------------------------------------
// Delaunay�O�p�`�����N���X
//  - http://tercel-sakuragaoka.blogspot.jp/2011/06/processingdelaunay.html
//    �̃R�[�h��C++�ɂ��āC�O�p�`�𒸓_+�ʑ��\���ɕς�������
//-----------------------------------------------------------------------------
class rxDelaunayTriangles
{  
	struct rxDTriangle
	{
		int idx[3];
		//rxTriangle triangle;
		bool enable;

		rxDTriangle(){}
		rxDTriangle(int a, int b, int c, bool e) : enable(e)
		{
			idx[0] = a; idx[1] = b; idx[2] = c;
		}

		//! �A�N�Z�X�I�y���[�^
		inline int& operator[](int i){ return idx[i]; }
		inline int operator[](int i) const { return idx[i]; }

		//! ==�I�y���[�^
		inline bool operator==(const rxDTriangle &a) const
		{
			return ((idx[0] == a[0] && idx[1] == a[1] && idx[2] == a[2]) || 
					(idx[0] == a[0] && idx[1] == a[2] && idx[2] == a[1]) || 
					(idx[0] == a[1] && idx[1] == a[0] && idx[2] == a[2]) || 
					(idx[0] == a[1] && idx[1] == a[2] && idx[2] == a[0]) || 
					(idx[0] == a[2] && idx[1] == a[0] && idx[2] == a[1]) || 
					(idx[0] == a[2] && idx[1] == a[1] && idx[2] == a[0]) );
		}
	};
	typedef vector<rxDTriangle> TriList;
	TriList m_vTriangles;

	vector<Vec2> m_vVertices;

public:
	/*!
	 * �R���X�g���N�^
	 */
	rxDelaunayTriangles()
	{ 
	}

	/*!
	 * Delaunay�O�p�`����
	 * @param[in] point_array �_�Q
	 * @param[in] minp,maxp �O�p�`�𐶐�����̈�
	 * @return �쐬���ꂽ�O�p�`��
	 */
	int DelaunayTriangulation(vector<Vec2> &point_array, Vec2 minp, Vec2 maxp)
	{  
		// �O�p�`���X�g�������� 
		m_vTriangles.clear();

		// �S�̂��͂ގO�p�`���쐬���ă��X�g�ɒǉ�
		rxDTriangle first_triangle = getCircumscribedTriangle(minp, maxp);
		m_vTriangles.push_back(first_triangle);

		// �_�𒀎��Y�����A�����I�ɎO�p�������s��  
		vector<Vec2>::iterator itr = point_array.begin();
		for(; itr != point_array.end(); ++itr){
			Vec2 p = *itr;
			TriList tmpTriangles;

			int pn = (int)m_vVertices.size();
			m_vVertices.push_back(p);

			// ���݂̎O�p�`���X�g����v�f��������o���āA  
			// �^����ꂽ�_���e�X�̎O�p�`�̊O�ډ~�̒��Ɋ܂܂�邩�ǂ�������  
			TriList::iterator jtr = m_vTriangles.begin();
			while(jtr != m_vTriangles.end()){
				rxDTriangle &t = *jtr;

				// �O�ډ~�����߂�  
				Vec2 c;
				double r;
				getCircumscribedCircles(t, c, r);

				// �ǉ����ꂽ�_���O�ډ~�����ɑ��݂���ꍇ�͎O�p�`�𕪊�
				if(norm(c-p) < r){
					// �V�����O�p�`���쐬���C�d���`�F�b�N���Ċi�[
					addTriangle(tmpTriangles, rxDTriangle(pn, t[0], t[1], true));  
					addTriangle(tmpTriangles, rxDTriangle(pn, t[1], t[2], true));  
					addTriangle(tmpTriangles, rxDTriangle(pn, t[2], t[0], true)); 

					// �O�ډ~����Ɏg�����O�p�`�����X�g����폜  
					jtr = m_vTriangles.erase(jtr);
				}
				else{
					jtr++;
				}
			}  

			// �ꎞ�n�b�V���̂����A�d���̂Ȃ����̂��O�p�`���X�g�ɒǉ�   
			jtr = tmpTriangles.begin();
			for(; jtr != tmpTriangles.end(); ++jtr){
				if(jtr->enable){
					m_vTriangles.push_back(*jtr);
				}
			}  
		}  

		// �O���O�p�`�ƒ��_�����L����O�p�`���폜  
		TriList::iterator jtr = m_vTriangles.begin();
		while(jtr != m_vTriangles.end()){
			rxDTriangle t = *jtr;
			if(hasCommonVertex(first_triangle, t)){
				jtr = m_vTriangles.erase(jtr);
			}
			else{
				jtr++;
			}
		}

		// �O���O�p�`�p�ɒǉ��������_���폜(�O�p�`�̒��_�C���f�b�N�X���C��)
		itr = m_vVertices.begin();
		itr = m_vVertices.erase(itr);
		itr = m_vVertices.erase(itr);
		itr = m_vVertices.erase(itr);
		if(!m_vTriangles.empty()){
			jtr = m_vTriangles.begin();
			while(jtr != m_vTriangles.end()){
				rxDTriangle &t = *jtr;
				for(int i = 0; i < 3; ++i){
					t[i] -= 3;
				}
				jtr++;
			}
		}


		return (int)m_vTriangles.size();
	}  


	//! �A�N�Z�X���\�b�h
	int GetTriangleNum(void) const { return (int)m_vTriangles.size(); }
	int GetVertexNum(void) const { return (int)m_vVertices.size(); }

	Vec2 GetVertex(int i) const { return m_vVertices[i]; }
	int  GetTriangle(int t, int v) const { return m_vTriangles[t][v]; }
	void GetTriangle(int t, int *idx) const
	{
		idx[0] = m_vTriangles[t][0];
		idx[1] = m_vTriangles[t][1];
		idx[2] = m_vTriangles[t][2];
	}


private:
	/*!
	 * �O�p�`���X�g���ɏd��������Ό��̎O�p�`��false�ɂ��āC�Ȃ���ΐV�����O�p�`t�����X�g�ɒǉ�
	 * @param[in] tri_list �O�p�`���X�g
	 * @param[in] t �ǉ��O�p�`
	 */
	void addTriangle(TriList &tri_list, rxDTriangle t)
	{  
		TriList::iterator itr = tri_list.begin();
		for(; itr != tri_list.end(); ++itr){
			if(*itr == t){
				itr->enable = false;
				break;
			}
		}
				
		if(itr == tri_list.end()){
			t.enable = true;
			tri_list.push_back(t);
		}
	} 

	/*!
	 * ���ʓ_�������ǂ����̔���
	 */
	bool hasCommonVertex(const rxDTriangle &t1, const rxDTriangle &t2)
	{
		return (t1[0] == t2[0] || t1[0] == t2[1] || t1[0] == t2[2] ||  
				t1[1] == t2[0] || t1[1] == t2[1] || t1[1] == t2[2] ||  
				t1[2] == t2[0] || t1[2] == t2[1] || t1[2] == t2[2] );  
	}


	/*!
	 * ��`������Ɋ܂ސ��O�p�`�̎Z�o
	 * @param[in] start,end ��`�̑Ίp�����\������2�[�_���W
	 * @return �O�p�`
	 */
	rxDTriangle getCircumscribedTriangle(Vec2 start, Vec2 end)
	{  
		if(end[0] < start[0]){  
			double tmp = start[0];  
			start[0] = end[0];  
			end[0] = tmp;  
		}  
		if(end[1] < start[1]){  
			double tmp = start[1];  
			start[1] = end[1];  
			end[1] = tmp;  
		}  

		// ��`���܂���~  
		Vec2 cen = 0.5*(end+start); 
		double rad = norm(cen-start)+0.01; 

		// �~�ɊO�ڂ��鐳�O�p�`(�~�̒��S=�d�S�C�Ӓ�2��3 r
		const double s3 = sqrt(3.0);
		Vec2 p1(cen[0]-s3*rad, cen[1]-rad);  
		Vec2 p2(cen[0]+s3*rad, cen[1]-rad);  
		Vec2 p3(cen[0], cen[1]+2*rad);

		int v1, v2, v3;
		v1 = (int)m_vVertices.size(); m_vVertices.push_back(p1);
		v2 = (int)m_vVertices.size(); m_vVertices.push_back(p2);
		v3 = (int)m_vVertices.size(); m_vVertices.push_back(p3);

		return rxDTriangle(v1, v2, v3, false);
	}  


	/*!
	 * �O�p�`�̊O�ډ~�����߂�
	 * @param[in] t �O�p�`
	 * @param[out] c,r 
	 */
	void getCircumscribedCircles(rxDTriangle t, Vec2 &c, double &r)
	{  
		double x1 = m_vVertices[t[0]][0];  
		double y1 = m_vVertices[t[0]][1];  
		double x2 = m_vVertices[t[1]][0];  
		double y2 = m_vVertices[t[1]][1];  
		double x3 = m_vVertices[t[2]][0];  
		double y3 = m_vVertices[t[2]][1];  

		double e = 2.0*((x2-x1)*(y3-y1) - (y2-y1)*(x3-x1));  
		double x = ((y3-y1)*(x2*x2-x1*x1+y2*y2-y1*y1) + (y1-y2)*(x3*x3-x1*x1+y3*y3-y1*y1))/e;
		double y = ((x1-x3)*(x2*x2-x1*x1+y2*y2-y1*y1) + (x2-x1)*(x3*x3-x1*x1+y3*y3-y1*y1))/e;
		c = Vec2(x, y);

		// �O�ډ~�̔��a�͒��S����O�p�`�̔C�ӂ̒��_�܂ł̋����ɓ����� 
		r = norm(c-m_vVertices[t[0]]);  
	}  
};


/*!
 * Delaunay�O�p�`���������s
 * @param[in] points ���ɂȂ�_�Q
 * @param[out] tris �O�p�`(���̓_�Q���X�g�ɑ΂���ʑ��\��)
 * @return �쐬���ꂽ�O�p�`�̐�
 */
inline static int CreateDelaunayTriangles(vector<Vec2> &points, vector< vector<int> > &tris)
{
	if(points.empty()) return 0;
	
	// �_�Q��AABB�����߂�
	Vec2 minp, maxp;
	minp = maxp = points[0];
	vector<Vec2>::iterator itr = points.begin();
	for(; itr != points.end(); ++itr){
		if((*itr)[0] < minp[0]) minp[0] = (*itr)[0];
		if((*itr)[1] < minp[1]) minp[1] = (*itr)[1];
		if((*itr)[0] > maxp[0]) maxp[0] = (*itr)[0];
		if((*itr)[1] > maxp[1]) maxp[1] = (*itr)[1];
	}

	// Delaunay�O�p�`����
	rxDelaunayTriangles delaunay;
	delaunay.DelaunayTriangulation(points, minp, maxp);

	// �������ꂽ�O�p�`�����i�[
	int tn = delaunay.GetTriangleNum();
	tris.resize(tn);
	for(int i = 0; i < tn; ++i){
		tris[i].resize(3);
		delaunay.GetTriangle(i, &tris[i][0]);
	}

	return 1;
}



#endif // #ifdef _RX_DELAUNAY_H_
