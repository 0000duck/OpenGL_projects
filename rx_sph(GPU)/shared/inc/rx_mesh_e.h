/*! 
  @file rx_mesh_e.h

  @brief �g�����b�V���\���̒�`
  		 - �G�b�W����ǉ�
  		 - ���_�C�G�b�W�C�ʂ̒ǉ��E�폜�ɑΉ�
 
  @author Makoto Fujisawa
  @date 2012-03
*/
// FILE --rx_mesh_e.h--

#ifndef _RX_MESH_E_H_
#define _RX_MESH_E_H_


//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
#include "rx_mesh.h"


//-----------------------------------------------------------------------------
// ���b�V���N���X
//-----------------------------------------------------------------------------
//! �G�b�W
struct rxEdge
{
	int v[2];		//!< �G�b�W���[�̒��_�C���f�b�N�X
	set<int> f;		//!< �G�b�W�ɂȂ���|���S�����
	double len;		//!< �G�b�W����
	int attribute;	//!< ����
};

//! �g���|���S���I�u�W�F�N�g(�G�b�W���ǉ�)
class rxPolygonsE : public rxPolygons
{
public:
	vector<rxEdge> edges;		//!< �G�b�W
	vector< set<int> > vedges;	//!< ���_�ɂȂ���G�b�W���
	vector< set<int> > vfaces;	//!< ���_�ɂȂ���|���S�����
	vector< set<int> > fedges;	//!< �|���S���Ɋ܂܂��G�b�W���

	vector<Vec3> vcol;			//!< ���_�F

public:
	//! �R���X�g���N�^
	rxPolygonsE() : rxPolygons() {}
	//! �f�X�g���N�^
	virtual ~rxPolygonsE(){}

	//! ������
	void Clear(void)
	{
		vertices.clear();
		normals.clear();
		faces.clear();
		materials.clear();
		edges.clear();
		vedges.clear();
		vfaces.clear();
		fedges.clear();
	}

};


//-----------------------------------------------------------------------------
// ���b�V���f�[�^�ҏW�֐�
//-----------------------------------------------------------------------------

/*!
 * �폜�t���O�t���̃|���S�����폜
 * @param[inout] src �ύX����|���S��
 * @param[in] del_attr �폜����ʂɓo�^���ꂽ����
 */
static int DeletePolygon(rxPolygonsE *src, int del_attr)
{
	vector<rxFace>::iterator iter = src->faces.begin();
	int d = 0;
	while(iter != src->faces.end()){
		if(iter->attribute == del_attr){
			iter = src->faces.erase(iter);
			d++;
		}
		else{
			++iter;
		}
	}
	return d;
}


/*!
 * �A���u�����I�y���[�^�Œ��_��Fairing
 * @param[inout] obj �g���|���S���I�u�W�F�N�g
 * @return �����̏ꍇ1
 */
static int VertexFairingByUmbrella(rxPolygonsE &obj)
{
	int vnum = (int)obj.vertices.size();
	if(!vnum || obj.vedges.empty()) return 0;

	for(int i = 0; i < vnum; ++i){
		Vec3 newpos = obj.vertices[i];

		// ���_�ɂȂ���G�b�W�𑖍�
		int en = 0;
		set<int>::iterator eiter = obj.vedges[i].begin();
		do{
			int v1, v2;
			v1 = obj.edges[*eiter].v[0];
			v2 = obj.edges[*eiter].v[1];

			newpos += obj.vertices[(v1 != i ? v1 : v2)];
			en++;
		}while(++eiter != obj.vedges[i].end());

		newpos /= (double)(en+1);

		obj.vertices[i] = newpos;
	}

	return 1;
}


/*!
 * ���_�ɐڑ�����|���S����T��
 * @param[inout] obj �g���|���S���I�u�W�F�N�g
 * @return �����̏ꍇ1
 */
static int SearchVertexFace(rxPolygonsE &obj)
{
	int vnum = (int)obj.vertices.size();
	if(!vnum) return 0;

	obj.vfaces.clear();
	obj.vfaces.resize(vnum);

	rxFace* face;
	int pnum = (int)obj.faces.size();

	// �S�|���S����T��
	for(int i = 0; i < pnum; ++i){
		face = &obj.faces[i];
		for(int j = 0; j < face->size(); ++j){
			int v = face->at(j);
			obj.vfaces[v].insert(i);
		}
	}

	return 1;
}

/*!
 * �G�b�W�f�[�^���쐬
 * @param[inout] obj �g���|���S���I�u�W�F�N�g
 * @return �G�b�W��
 */
static int SearchEdge(rxPolygonsE &obj)
{
	int edge_count = 0;
	int vnum = (int)obj.vertices.size();
	int pnum = (int)obj.faces.size();

	obj.vedges.clear();
	obj.vedges.resize(vnum);
	obj.fedges.clear();
	obj.fedges.resize(pnum);
	obj.edges.clear();

	rxFace* face;
	int vert_idx[2];


	// �S�|���S����T��
	for(int i = 0; i < pnum; ++i){
		face = &obj.faces[i];
		vnum = face->size();

		for(int j = 0; j < vnum; ++j){
			// �G�b�W���_1
			vert_idx[0] = face->at(j);
			// �G�b�W���_2
			vert_idx[1] = (j == vnum-1) ? face->at(0) : face->at(j+1);

			// �d������Ő��̃`�F�b�N
			bool overlap = false;
			set<int>::iterator viter;

			// �G�b�W���_1�̃`�F�b�N
			if((int)obj.vedges[vert_idx[0]].size() > 0){
				// ���_�ɂȂ����Ă���G�b�W(���ɓo�^���ꂽ����)�𒲂ׂ�
				viter = obj.vedges[vert_idx[0]].begin();
				do{
					int v1, v2;
					v1 = obj.edges[*viter].v[0];
					v2 = obj.edges[*viter].v[1];

					if( (vert_idx[0] == v1 && vert_idx[1] == v2) || (vert_idx[1] == v1 && vert_idx[0] == v2) ){
						overlap = true;
						obj.edges[*viter].f.insert(i);	// �ʂ��G�b�W�ɉ�����
						obj.fedges[i].insert(*viter);	// �G�b�W��ʂɉ�����
					}
				}while(++viter != obj.vedges[vert_idx[0]].end());
			}

			// �G�b�W���_2�̃`�F�b�N
			if((int)obj.vedges[vert_idx[1]].size() > 0){
				// ���_�ɂȂ����Ă���G�b�W(���ɓo�^���ꂽ����)�𒲂ׂ�
				viter = obj.vedges[vert_idx[1]].begin();
				do{
					int v1, v2;
					v1 = obj.edges[*viter].v[0];
					v2 = obj.edges[*viter].v[1];

					if( (vert_idx[0] == v1 && vert_idx[1] == v2) || (vert_idx[1] == v1 && vert_idx[0] == v2) ){
						overlap = true;
						obj.edges[*viter].f.insert(i);	// �ʂ��G�b�W�ɉ�����
						obj.fedges[i].insert(*viter);	// �G�b�W��ʂɉ�����
					}
				}while(++viter != obj.vedges[vert_idx[1]].end());
			}

			// �d������Ő��Ȃ��̏ꍇ�C�V�K�ɒǉ�
			if(!overlap){
				obj.edges.push_back(rxEdge());
				obj.edges[edge_count].v[0] = vert_idx[0];
				obj.edges[edge_count].v[1] = vert_idx[1];
				obj.edges[edge_count].f.insert(i);

				obj.vedges[vert_idx[0]].insert(edge_count);
				obj.vedges[vert_idx[1]].insert(edge_count);
				obj.fedges[i].insert(edge_count);

				edge_count++;
			}

		}
	}

	return edge_count;
}


/*!
 * �G�b�W�������v�Z
 * @param[inout] obj �g���|���S���I�u�W�F�N�g
 * @return �G�b�W��
 */
static int CalEdgeLength(rxPolygonsE &obj)
{
	int en = (int)obj.edges.size();

	if(en == 0) return 0;

	int vn = (int)obj.vertices.size();

	rxEdge *edge;
	for(int i = 0; i < en; ++i){
		edge = &obj.edges[i];

		int v0 = edge->v[0];
		int v1 = edge->v[1];

		edge->len = norm(obj.vertices[v0]-obj.vertices[v1]);
	}

	return 1;
}


#endif // #ifndef _RX_MESH_E_H_
