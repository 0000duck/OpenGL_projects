/*!
  @file rx_sph_solid_poly.cpp
	
  @brief SPH�p�ő̒�`
		 �|���S��+�A�֐�(OpenVDB�ō쐬)
 
  @author Makoto Fujisawa
  @date 2014-02
*/
//#define RX_USE_OPENVDB

#ifdef RX_USE_OPENVDB
#pragma comment(lib, "Half.lib")
#pragma comment(lib, "tbb.lib")
#pragma comment(lib, "openvdb.lib")
//#pragma comment(lib, "zlib.lib")
#endif

//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------

#ifdef RX_USE_OPENVDB
// OpenVDB
#include <openvdb/openvdb.h>
#include <openvdb/Exceptions.h>

#include <openvdb/tools/MeshToVolume.h>
#include <openvdb/tools/VolumeToMesh.h>
#include <openvdb/util/Util.h>

#include <openvdb/tools/Interpolation.h>
#endif


#include "rx_sph_solid.h"

// �\�ʃp�[�e�B�N���z�u
#include "rx_particle_on_surf.h"

// 3D���b�V��
#include "rx_model.h"

#ifdef RX_USE_OPENVDB
openvdb::FloatGrid::Ptr g_pGrid = 0;

/*!
 * �|���S�����畄���t��������̐���
 * @param[in] poly �|���S���f�[�^
 * @param[in] gw �O���b�h������
 * @return �����t��������(�O���b�h)
 */
static openvdb::FloatGrid::Ptr MakeSDF(const rxPolygons &poly, double gw)
{
	using namespace openvdb;

	std::vector<Vec3s> points;
	std::vector<Vec3I> triangles;
	std::vector<Vec4I> quads;

	for(int i = 0; i < (int)poly.vertices.size(); ++i){
		Vec3 v = poly.vertices[i];
		points.push_back(Vec3s(v[0], v[1], v[2]));
	}

	for(int i = 0; i < (int)poly.faces.size(); ++i){
		rxFace f = poly.faces[i];
		triangles.push_back(Vec3I(f[0], f[1], f[2]));
	}

	float bandwidth[2] = {0.2f, 0.2f};	// exterior/interior narrow-band width;
	bandwidth[0] = gw*10;
	bandwidth[1] = gw*10;
	openvdb::math::Transform xform = *math::Transform::createLinearTransform(gw);

	return tools::meshToSignedDistanceField<FloatGrid>(xform, points, triangles, quads, bandwidth[0], bandwidth[1]);
}
#endif




//-----------------------------------------------------------------------------
// rxSolidPolygon�N���X�̎���
//-----------------------------------------------------------------------------
/*!
 * �R���X�g���N�^
 * @param[in] filename �|���S���t�@�C����(OBJ,WRL�Ȃ�)
 * @param[in] cen �ő̃I�u�W�F�N�g���S���W
 * @param[in] ext �ő̃I�u�W�F�N�g�̑傫��(�ӂ̒�����1/2)
 * @param[in] ang �ő̃I�u�W�F�N�g�̊p�x(�I�C���[�p)
 * @param[in] h �L�����a(�����Z����)
 * @param[in] aspect ���|���S���̃A�X�y�N�g���ۂ��ǂ���(0��ext�ɍ��킹�ĕύX)
 */
rxSolidPolygon::rxSolidPolygon(const string &fn, Vec3 cen, Vec3 ext, Vec3 ang, double h, int aspect)
{
	m_vMin = -ext;
	m_vMax =  ext;
	m_vMassCenter = cen;

	m_hVrts = 0; 
	m_hTris = 0; 
	m_iNumTris = 0;

	m_iSgn = 1;
	m_iName = RXS_POLYGON;

	// �|���S��������
	if(!m_Poly.vertices.empty()){
		m_Poly.vertices.clear();
		m_Poly.normals.clear();
		m_Poly.faces.clear();
		m_Poly.materials.clear();
	}
	
	// HACK:�|���S�����f���ǂݍ���
	readFile(fn, m_Poly);
	
	if(m_Poly.vertices.empty()){
		return;
	}

	// �|���S�����_��AABB�Ƀt�B�b�g
	if(RXFunc::IsZeroVec(ang)){
		fitVertices(cen, ext, m_Poly.vertices);
	}
	else{
		AffineVertices(m_Poly, cen, ext, ang);
	}

	// AABB�̒T��
	FindBBox(m_vMin, m_vMax, m_Poly.vertices);
	m_vMassCenter = 0.5*(m_vMin+m_vMax);
	Vec3 dim = m_vMax-m_vMin;
	m_vMin = -0.5*dim;
	m_vMax =  0.5*dim;

	cout << "solid min-max : " << m_vMin << " - " << m_vMax << endl;
	cout << "      mc      : " << m_vMassCenter << endl;


	// �|���S�����_�@���̌v�Z
	if(m_Poly.normals.empty()){
		CalVertexNormals(m_Poly);
	}

	int vn = (int)m_Poly.vertices.size();
	int n = (int)m_Poly.faces.size();

	if(m_hVrts) delete [] m_hVrts;
	if(m_hTris) delete [] m_hTris;
	m_hVrts = new RXREAL[vn*3];
	m_hTris = new int[n*3];

	for(int i = 0; i < vn; ++i){
		for(int j = 0; j < 3; ++j){
			m_hVrts[3*i+j] = m_Poly.vertices[i][j];
		}
	}

	for(int i = 0; i < n; ++i){
		for(int j = 0; j < 3; ++j){
			m_hTris[3*i+j] = m_Poly.faces[i][j];
		}
	}

	m_iNumVrts = vn;
	m_iNumTris = n;
	RXCOUT << "the number of triangles : " << m_iNumTris << endl;

	// �����Z�������p��1%�����g������BBox�𐶐�
	Vec3 minp = m_vMassCenter+m_vMin;
	Vec3 maxp = m_vMassCenter+m_vMax;
	CalExtendedBBox(minp, maxp, 0.05);


	// �ߖT�T���Z��
	m_pNNGrid = new rxNNGrid(DIM);

	// �����Z���ݒ�
	m_pNNGrid->Setup(minp, maxp, h, n);

	// �����Z���Ƀ|���S����o�^
	m_pNNGrid->SetPolygonsToCell(m_hVrts, m_iNumVrts, m_hTris, m_iNumTris);
	
	// ���C�g���Ȃǂł̕`��p�Ɍő̃I�u�W�F�N�g���b�V�����t�@�C���ۑ����Ă���
	string outfn = RX_DEFAULT_MESH_DIR+"solid_boundary.obj";
	rxOBJ saver;
	rxMTL mtl;	// �ގ���empty�̂܂�
	if(saver.Save(outfn, m_Poly.vertices, m_Poly.normals, m_Poly.faces, mtl)){
		RXCOUT << "saved the mesh to " << outfn << endl;
	}

#ifdef RX_USE_OPENVDB
	g_pGrid = 0;
	openvdb::initialize();
	g_pGrid = MakeSDF(m_Poly, 0.02);
#endif
}

/*!
 * �f�X�g���N�^
 */
rxSolidPolygon::~rxSolidPolygon()
{
	if(m_pNNGrid) delete m_pNNGrid;
	if(m_hVrts) delete [] m_hVrts;
	if(m_hTris) delete [] m_hTris;
}

/*!
 * �����l�v�Z(�_�Ƃ̋���)
 * @param[in] pos �O���[�o�����W�ł̈ʒu
 * @param[out] col �����Ȃǂ̏��(�Փˏ��)
 */
bool rxSolidPolygon::GetDistance(const Vec3 &pos, rxCollisionInfo &col)
{
	return GetDistanceR(pos, 0.0, col);
}
bool rxSolidPolygon::GetDistance(const Vec3 &pos0, const Vec3 &pos1, rxCollisionInfo &col)
{
	return GetDistanceR(pos0, pos1, 0.0, col);
}



/*!
 * �����l�v�Z
 * @param[in] pos �O���[�o�����W�ł̈ʒu
 * @param[in] r ���̔��a
 * @param[out] col �����Ȃǂ̏��(�Փˏ��)
 */
bool rxSolidPolygon::GetDistanceR(const Vec3 &pos, const double &r, rxCollisionInfo &col)
{
#ifdef RX_USE_OPENVDB
	if(g_pGrid){
		openvdb::tools::GridSampler<openvdb::FloatGrid, openvdb::tools::BoxSampler> sampler(*g_pGrid);
		openvdb::Vec3R ijk(pos[0], pos[1], pos[2]);
		openvdb::FloatGrid::ValueType v0;
		v0 = sampler.wsSample(ijk);
		col.Penetration() = -((RXREAL)(v0)+m_fOffset*2);

		openvdb::FloatGrid::Accessor accessor = g_pGrid->getAccessor();
		openvdb::Coord nijk;
		openvdb::Vec3d npos(pos[0], pos[1], pos[2]);
		npos = g_pGrid->worldToIndex(npos);
		nijk[0] = int(npos[0]);
		nijk[1] = int(npos[1]);
		nijk[2] = int(npos[2]);
		accessor.getValue(nijk);
		openvdb::Vec3d nrm = openvdb::math::ISGradient<openvdb::math::CD_2ND>::result(accessor, nijk);
		double length = nrm.length();
		if(length > 1.0e-6){
			nrm *= 1.0/length;
		}
		col.Normal() = Vec3(nrm.x(), nrm.y(), nrm.z());
		return true;
	}
	else{
		return GetDistanceR(pos, pos, r, col);
	}
#else
	return GetDistanceR(pos, pos, r, col);
#endif
}

/*!
 * �����l�v�Z
 * @param[in] pos �O���[�o�����W�ł̈ʒu
 * @param[in] r ���̔��a
 * @param[out] col �����Ȃǂ̏��(�Փˏ��)
 */
bool rxSolidPolygon::GetDistanceR(const Vec3 &pos0, const Vec3 &pos1, const double &r, rxCollisionInfo &col)
{
	Vec3 dir = Unit(pos1-pos0);
	int c = 0;
	double min_d = RX_FEQ_INF;
	set<int> polys_in_cell;	// �d���Ȃ��R���e�i
	int cn = m_pNNGrid->GetNNPolygons(pos0, polys_in_cell, r);	// r=0�ł�pos���܂܂��Z��+1���͍͂Œ���T������
	cn    += m_pNNGrid->GetNNPolygons(pos1+dir*r, polys_in_cell, r);
	if(cn){ 
		set<int>::iterator p = polys_in_cell.begin();
		for(; p != polys_in_cell.end(); ++p){
			int pidx = *p;

			// �O�p�`�|���S�����\�����钸�_�C���f�b�N�X
			int vidx[3];
			vidx[0] = m_hTris[3*pidx+0];
			vidx[1] = m_hTris[3*pidx+1];
			vidx[2] = m_hTris[3*pidx+2];

			// �O�p�`�|���S���̒��_���W
			Vec3 vrts[3];
			vrts[0] = Vec3(m_hVrts[3*vidx[0]], m_hVrts[3*vidx[0]+1], m_hVrts[3*vidx[0]+2]);
			vrts[1] = Vec3(m_hVrts[3*vidx[1]], m_hVrts[3*vidx[1]+1], m_hVrts[3*vidx[1]+2]);
			vrts[2] = Vec3(m_hVrts[3*vidx[2]], m_hVrts[3*vidx[2]+1], m_hVrts[3*vidx[2]+2]);

			Vec3 cp, n;
			double d;
			//n = Unit(cross(vrts[1]-vrts[0], vrts[2]-vrts[0]));

			if(intersectSegmentTriangle(pos0, pos1+dir*r, vrts[0], vrts[1], vrts[2], cp, n) == 1){
				d = length(pos0-cp);
				if(d < min_d){
					col.Penetration() = d;
					col.Normal() = n;
					//double l = r/(dot(-n, dir));
					col.Contact() = cp;//-l*dir;
					min_d = d;
					c++;
				}
			}
		}
	}
	
	return (c == 0 ? false : true);
}


/*!
 * �|���S�����ʂ܂ł̋������v�Z
 * @param[in] x �v�Z���W
 * @param[in] p �|���S���ԍ�
 * @param[out] dist �����t������
 */
int rxSolidPolygon::getDistanceToPolygon(Vec3 x, int p, double &dist)
{
	vector<int> &vs = m_Poly.faces[p].vert_idx;
	Vec3 pnrm = Unit(cross(m_Poly.vertices[vs[1]]-m_Poly.vertices[vs[0]], m_Poly.vertices[vs[2]]-m_Poly.vertices[vs[0]]));
	dist = dot(x-m_Poly.vertices[vs[0]], pnrm);
	return 0;
}



/*!
 * �\�ʋȗ��v�Z
 *  - �P�ɉA�֐��̋��2�K�������v�Z���Ă��邾��
 * @param[in] pos �v�Z�ʒu
 * @param[out] k �ȗ�
 */
bool rxSolidPolygon::GetCurvature(const Vec3 &pos, double &k)
{
	return CalCurvature(pos, k, &rxSolidPolygon::GetDistance_s, this);
}

/*!
 * OpenGL�ϊ��s���ݒ�
 */
void rxSolidPolygon::SetGLMatrix(void)
{
	glTranslatef(m_vMassCenter[0], m_vMassCenter[1], m_vMassCenter[2]);
	glMultMatrixd(m_matRot.GetValue());
}

/*!
 * OpenGL�ɂ��`��
 * @param[in] drw �`��t���O(drw&2 == 1�Ń��C���t���[���`��)
 */
void rxSolidPolygon::Draw(int drw)
{
	glPushMatrix();

	//SetGLMatrix();
	
	// ���b�V��
	glEnable(GL_LIGHTING);
	glColor4d(0.0, 1.0, 0.0, 1.0);
	m_Poly.Draw(drw & 14);

	glPopMatrix();
}

/*!
 * �����Z���Ɋi�[���ꂽ�|���S�������擾
 * @param[in] gi,gj,gk �Ώە����Z��
 * @param[out] polys �|���S��
 * @return �i�[�|���S����
 */
int rxSolidPolygon::GetPolygonsInCell(int gi, int gj, int gk, set<int> &polys)
{
	return m_pNNGrid->GetPolygonsInCell(gi, gj, gk, polys);
}

/*!
 * �����Z�����̃|���S���̗L���𒲂ׂ�
 * @param[in] gi,gj,gk �Ώە����Z��
 * @return �|���S�����i�[����Ă����true
 */
bool rxSolidPolygon::IsPolygonsInCell(int gi, int gj, int gk)
{
	return m_pNNGrid->IsPolygonsInCell(gi, gj, gk);
}



/*!
 * OBJ�t�@�C���ǂݍ���
 * @param[in] filename wrl�t�@�C���̃p�X
 */
void rxSolidPolygon::readOBJ(const string filename, rxPolygons &polys)
{
	if(!polys.vertices.empty()){
		polys.vertices.clear();
		polys.normals.clear();
		polys.faces.clear();
		polys.materials.clear();
	}
	rxOBJ obj;
	if(obj.Read(filename, polys.vertices, polys.normals, polys.faces, polys.materials, true)){
		RXCOUT << filename << " have been read." << endl;

		RXCOUT << " the number of vertex   : " << polys.vertices.size() << endl;
		RXCOUT << " the number of normal   : " << polys.normals.size() << endl;
		RXCOUT << " the number of polygon  : " << polys.faces.size() << endl;
		RXCOUT << " the number of material : " << polys.materials.size() << endl;

		polys.open = 1;
	}
}


int rxSolidPolygon::readFile(const string filename, rxPolygons &polys)
{
	string ext = GetExtension(filename);
	if(ext == "obj"){
		readOBJ(filename, polys);
	}

	return polys.open;
}



/*!
 * ���_���AABB�ɍ����悤��Fit������(���̌`��̏c����͈ێ�)
 * @param[in] ctr AABB���S���W
 * @param[in] sl  AABB�̕ӂ̒���(1/2)
 * @param[in] vec_set ���_��
 * @param[in] start_index,end_index ���_��̌����͈�
 */
bool rxSolidPolygon::fitVertices(const Vec3 &ctr, const Vec3 &sl, vector<Vec3> &vec_set, int aspect)
{
	if(vec_set.empty()) return false;

	int n = (int)vec_set.size();

	// ���݂�BBox�̑傫���𒲂ׂ�
	Vec3 maxp = vec_set[0];
	Vec3 minp = vec_set[0];

	for(int i = 1; i < n; ++i){
		if(vec_set[i][0] > maxp[0]) maxp[0] = vec_set[i][0];
		if(vec_set[i][1] > maxp[1]) maxp[1] = vec_set[i][1];
		if(vec_set[i][2] > maxp[2]) maxp[2] = vec_set[i][2];
		if(vec_set[i][0] < minp[0]) minp[0] = vec_set[i][0];
		if(vec_set[i][1] < minp[1]) minp[1] = vec_set[i][1];
		if(vec_set[i][2] < minp[2]) minp[2] = vec_set[i][2];
	}

	Vec3 ctr0, sl0;
	sl0  = (maxp-minp)/2.0;
	ctr0 = (maxp+minp)/2.0;

	Vec3 size_conv;
	
	if(aspect){
		int max_axis = ( ( (sl0[0] > sl0[1]) && (sl0[0] > sl0[2]) ) ? 0 : ( (sl0[1] > sl0[2]) ? 1 : 2 ) );
		int min_axis = ( ( (sl0[0] < sl0[1]) && (sl0[0] < sl0[2]) ) ? 0 : ( (sl0[1] < sl0[2]) ? 1 : 2 ) );
		size_conv = Vec3(sl[max_axis]/sl0[max_axis]);
	}
	else{
		size_conv = sl/sl0;
	}

	// �S�Ă̒��_��bbox�ɂ��킹�ĕϊ�
	for(int i = 0; i < n; ++i){
		vec_set[i] = (vec_set[i]-ctr0)*size_conv+ctr;
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
int rxSolidPolygon::intersectSegmentTriangle(Vec3 P0, Vec3 P1,			// Segment
									Vec3 V0, Vec3 V1, Vec3 V2,	// Triangle
									Vec3 &I, Vec3 &n)			// Intersection point (return)
{
	// �O�p�`�̃G�b�W�x�N�g���Ɩ@��
	Vec3 u = V1-V0;
	Vec3 v = V2-V0;
	n = Unit(cross(u, v));
	if(RXFunc::IsZeroVec(n)){
		return -1;	// �O�p�`��"degenerate"�ł���(�ʐς�0)
	}

	// ����
	Vec3 dir = P1-P0;
	double a = dot(n, P0-V0);
	double b = dot(n, dir);
	if(fabs(b) < 1e-10){	// �����ƎO�p�`���ʂ����s
		if(a == 0){
			return 2;	// ���������ʏ�
		}
		else{
			return 0;	// ��_�Ȃ�
		}
	}

	// ��_�v�Z

	// 2�[�_�����ꂼ��قȂ�ʂɂ��邩�ǂ����𔻒�
	float r = -a/b;
	if(a < 0){
		return 0;
	}

	if(r < 0.0){
		return 0;
	}
	else{
		if(fabs(a) > fabs(b)){
			return 0;
		}
		else{
			if(b > 0){
				return 0;
			}
		}
	}

	// �����ƕ��ʂ̌�_
	I = P0+r*dir;

	// ��_���O�p�`���ɂ��邩�ǂ����̔���
	double uu, uv, vv, wu, wv, D;
	uu = dot(u, u);
	uv = dot(u, v);
	vv = dot(v, v);
	Vec3 w = I-V0;
	wu = dot(w, u);
	wv = dot(w, v);
	D = uv*uv-uu*vv;

	double s, t;
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