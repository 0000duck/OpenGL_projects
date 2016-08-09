/*!
  @file rx_3ds.h
	
  @brief 3DS File Input/Output
 
  @author Makoto Fujisawa
  @date   2011
*/

#ifndef _RX_3DS_H_
#define _RX_3DS_H_


//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------
#include "rx_mesh.h"

//-----------------------------------------------------------------------------
// Name Space
//-----------------------------------------------------------------------------
using namespace std;


//-----------------------------------------------------------------------------
// rx3DS�N���X�̐錾 - 3DS�`���̓ǂݍ���
//-----------------------------------------------------------------------------
class rx3DS
{
public:
	//! �R���X�g���N�^
	rx3DS();
	//! �f�X�g���N�^
	~rx3DS();

	/*!
	 * 3DS�t�@�C���ǂݍ���
	 * @param[in] file_name �t�@�C����(�t���p�X)
	 * @param[out] vrts ���_���W
	 * @param[out] vnms ���_�@��
	 * @param[out] poly �|���S��
	 * @param[out] mats �ގ����
	 * @param[in] triangle �|���S���̎O�p�`�����t���O
	 */
	bool Read(string file_name, vector<Vec3> &vrts, vector<Vec3> &vnms, vector<rxFace> &plys, 
			  rxMTL &mats, bool triangle = true);

	/*!
	 * 3DS�t�@�C����������(������)
	 * @param[in] file_name �t�@�C����(�t���p�X)
	 * @param[in] vrts ���_���W
	 * @param[in] vnms ���_�@��
	 * @param[in] plys �|���S��
	 * @param[in] mats �ގ����
	 */
	bool Save(string file_name, const vector<Vec3> &vrts, const vector<Vec3> &vnms, const vector<rxFace> &plys, 
			  const rxMTL &mats);

public:
	struct rxChunk
	{
		unsigned short ID;		//!< �`�����N�̎��ʎq(2�o�C�g)
		unsigned int   Length;	//!< �`�����N�̒���(4�o�C�g) : �`�����N�f�[�^+�T�u�`�����N(6+n+m)
	};

	struct rxMap3DS
	{
		string Filename;
		int Tiling;
		float Blur;
		float Scale[2];
		float Offset[2];
		float Rotation;
		float Tint_1[3];
		float Tint_2[3];
		float Tint_r[3];
		float Tint_g[3];
		float Tint_b[3];
	};

	struct rxMaterial3DS
	{
		string Name;
		Vec3 Ambient;
		Vec3 Diffuse;
		Vec3 Specular;
		float Shininess;
		float ShinStrength;
		float Transparency;

		float FallOff;
		bool  UseFallOff;
		float SelfIllum;
		bool  UseSelfIllum;
		float Blur;
		bool  UseBlur;

		int Shading;
		bool TwoSided;
		bool MapDecal;
		bool IsAdditive;
		bool FaceMap;
		bool PhongSoft;

		bool UseWire;
		bool UseWireAbs;
		float WireSize;

		rxMap3DS TextureMap;
		rxMap3DS TextureMask;
		rxMap3DS TextureMap2;
		rxMap3DS TextureMask2;
		rxMap3DS OpacityMap;
		rxMap3DS OpacityMask;
		rxMap3DS BumpMap;
		rxMap3DS BumpMask;
		rxMap3DS SpecularMap;
		rxMap3DS SpecularMask;
		rxMap3DS ShininessMap;
		rxMap3DS ShininessMask;
		rxMap3DS SelfIllumMap;
		rxMap3DS SelfIllumMask;
		rxMap3DS ReflectionMap;
		rxMap3DS ReflectionMask;

		int AutoReflectionMapAA;
		int AutoReflectionMapFlags;
		int AutoReflectionMapSize;
		int AutoReflectionMapFrameStep;
	};

	struct rxLight3DS
	{
		Vec3 Pos;			//!< �������W
		Vec3 Color;			//!< �����F

		bool Enable;		//!< ���C�gON/OFF
		float OuterRange;
		float InnerRange;
		float Multiplier;
		float Attenuation;

		bool Spot;			//!< �X�|�b�g���C�gON/OFF
		Vec3 Target;		//!< �X�|�b�g���C�g�̏Ǝˈʒu
		float HotSpot;		//!< �X�|�b�g���C�g�̃z�b�g�X�|�b�g
		float FallOff;		//!< �X�|�b�g���C�g�̌���
		float Rotation;		//!< �X�|�b�g���C�g�̉�]
		bool Shadowed;
		float ShadowBias;
		float ShadowFilter;
		int ShadowSize;
		bool SeeCone;
		bool RectangularSpot;
		float SpotAspect;
		bool UseProjector;
		string Projector;
		bool SpotOvershoot;
		float RayBias;
		bool RayShadows;
	};

	struct rxCamera3DS
	{
		Vec3 Pos;			//!< ���_�ʒu
		Vec3 LookAt;		//!< �����_�ʒu
		float Rotation;		//!< ��]
		float FOV;			//!< FOV

		bool SeeCone;		//!< 
		float NearRange;	//!< Near
		float FarRange;		//!< Far
	};

	struct rxView3DS
	{
		int Type;
		int AxisLock;
		int Pos[2];
		int Size[2];
		float Zoom;
		float Center[3];
		float HorizontalAng;
		float VerticalAng;
		char Camera[11];
	};

	struct rxViewport3DS
	{
		int LayoutStyle;
		int LayoutActive;
		int LayoutSwap;
		int LayoutSwapPrior;
		int LayoutSwapView;
		int LayoutPos[2];
		int LayoutSize[2];
		int LayoutNViews;
		rxView3DS LayoutViews[32];

		int DefaultType;
		Vec3 DefaultPos;
		float DefaultWidth;
		float DefaultHorizontalAng;
		float DefaultVerticalAng;
		float DefaultRollAng;
		char DefaultCamera[11];
	};

	struct rxBackground3DS
	{
		bool UseBitmap;
		bool UseColor;
		bool UseGradient;

		string Bitmap;
		Vec4 Color;
		Vec3 Gradient;
	};

	struct rxFog3DS
	{
		bool UseFog;
		bool UseLayer;
		bool UseDistanceCue;

		float NearPlane;
		float NearDensity;
		float FarPlane;
		float FarDensity;
		Vec3  Color;
		bool UseFogBackground;

		float LayerNearY;
		float LayerFarY;
		float LayerDensity;
		long  LayerFlags;
		Vec3  LayerColor;

		float DistCueNearPlane;
		float DistCueNearDimming;
		float DistCueFarPlane;
		float DistCueFarDimming;
		bool UseDistCueBackground;
	};

	struct rxShadow3DS
	{
		int MapSize;
		float LowBias;
		float HiBias;
		float RayBias;
		float Filter;
		int Samples;
	};

	struct rxMesh3DS
	{
		vector<Vec2> TexCoords;			//!< �e�N�X�`�����W(for ���_)
		vector<int> VertexFlags;
		float TransMat[4][4];			//!< �ϊ��s��

		int Color;
		Vec2 MapTile;
		Vec3 MapPos;
		float MapScale;
		float MapMatrix[4][4];
		Vec2 MapPlane;
		float MapCylinderHeight;

		vector<int> SmoothingGroup;

		string BoxFront;
		string BoxBack;
		string BoxLeft;
		string BoxRight;
		string BoxTop;
		string BoxBottom;

		rxMesh3DS()
		{
			for(int i = 0; i < 4; ++i){
				for(int j = 0; j < 4; ++j){
					TransMat[i][j] = MapMatrix[i][j] = (i == j ? 1.0f : 0.0f);
				}
			}
		}
	};

public:
	//
	// �����o�ϐ�
	//
	int m_iMeshVersion;					//!< ���b�V���o�[�W����
	float m_fMasterScale;				//!< �S�̂̃X�P�[�����O

	vector<rxLight3DS> m_vLights;		//!< ����
	vector<rxCamera3DS> m_vCameras;		//!< ���_

	vector<rxMaterial3DS> m_vMats;		//!< �ގ�

	rxViewport3DS m_Viewport;			//!< �r���[�|�[�g

	Vec3 m_v3ConstructionPlane;			//!< ��b����

	Vec4 m_v4Ambient;					//!< ����
	rxBackground3DS m_Background;		//!< �w�i
	rxShadow3DS m_Shadow;				//!< �e
	rxFog3DS m_Fog;						//!< �t�H�O

	vector<rxMesh3DS> m_Mesh;			//!< 
	int m_iVertexOffset;				//!< �����̎O�p�`���b�V���I�u�W�F�N�g������Ƃ��̒��_�I�t�Z�b�g�l

	map<int, int> m_mapDefaultView;

	int m_iObjectFlags;					//!< �I�u�W�F�N�g�t���b�O

private:
	//! �`�����N�̎��ʎq
	enum rxChunkID
	{
		MAIN3DS 			= 0x4D4D, 

		EDIT3DS 			= 0x3D3D, 
		KEYF3DS 			= 0xB000, 

		// EDIT3DS
		EDIT_MESH_VERSION		= 0x3D3E,
		EDIT_MASTER_SCALE		= 0x0100,

		EDIT_BITMAP 			= 0x1100,
		EDIT_USE_BITMAP			= 0x1101,
		EDIT_BACKGROUND			= 0x1200,
		EDIT_USE_BACKGROUND		= 0x1201,
		EDIT_V_GRADIENT			= 0x1300,
		EDIT_USE_V_GRADIENT		= 0x1301,

		EDIT_SHADOW_LO_BIAS		= 0x1400,
		EDIT_SHADOW_HI_BIAS		= 0x1410,
		EDIT_SHADOW_MAPSIZE		= 0x1420,
		EDIT_SHADOW_SAMPLES		= 0x1430,
		EDIT_SHADOW_RANGE		= 0x1440,
		EDIT_SHADOW_FILTER		= 0x1450,
		EDIT_SHADOW_RAY_BIAS	= 0x1460, 

		EDIT_O_CONSTS 			= 0x1500,

		EDIT_AMBIENT_LIGHT		= 0x2100,

		EDIT_FOG	 			= 0x2200,
		  // EDIT_FOG
		  FOG_BACKGROUND		= 0x2210,
		EDIT_USE_FOG 			= 0x2201,
		EDIT_DISTANCE_CUE		= 0x2300,
		  // EDIT_DISTANCE_CUE
		  DCUE_BACKGROUND		= 0x2310, 
		EDIT_USE_DISTANCE_CUE	= 0x2301,
		EDIT_LAYER_FOG			= 0x2302,
		EDIT_USE_LAYER_FOG		= 0x2303,

		EDIT_DEFAULT_VIEW		= 0x3000,
		  // EDIT_DEFAULT_VIEW
		  VIEW_TOP				= 0x3010,
		  VIEW_BOTTOM			= 0x3020,
		  VIEW_LEFT				= 0x3030,
		  VIEW_RIGHT			= 0x3040,
		  VIEW_FRONT			= 0x3050,
		  VIEW_BACK				= 0x3060,
		  VIEW_USER				= 0x3070,
		  VIEW_CAMERA			= 0x3080,
		EDIT_VIEWPORT_LAYOUT	= 0x7001,
		  // EDIT_VIEWPORT_LAYOUT
		  VIEWPORT_DATA			= 0x7011,
		  VIEWPORT_DATA_3		= 0x7012,
		  VIEWPORT_SIZE			= 0x7020,

		EDIT_OBJECT 			= 0x4000,	//!< �I�u�W�F�N�g
		  // EDIT_OBJECT
		  OBJ_TRIMESH 				= 0x4100,
		    // OBJ_TRIMESH
		    TRI_VERTICES			= 0x4110,
		    TRI_VERTICES_FLAGS		= 0x4111,
		    TRI_FACES				= 0x4120,
		      // TRI_FACES
		      FACE_MAT_GROUP		= 0x4130,
		      FACE_SMOOTH_GROUP		= 0x4150,
		      FACE_MSH_BOXMAP		= 0x4190,
		    TRI_TEXCOORDS			= 0x4140,
		    TRI_TRANSLATION			= 0x4160,
		    TRI_MESH_COLOR			= 0x4165,
		    TRI_MESH_TEXTURE_INFO	= 0x4170,
		  OBJ_DIRECT_LIGHT			= 0x4600,
		    // OBJ_DIRECT_LIGHT
		    LIGHT_OFF				= 0x4620,
		    LIGHT_OUTER_RANGE		= 0x465A,
		    LIGHT_INNER_RANGE		= 0x4659,
		    LIGHT_MULTIPLIER		= 0x465B,
		    LIGHT_ATTENUATE			= 0x4625,
		    LIGHT_SPOT				= 0x4610,
		      // LIGHT_SPOT
		      SPOTLIT_ROLL			= 0x4656,
		      SPOTLIT_SHADOWED		= 0x4630,
		      SPOTLIT_LOCAL_SHADOW	= 0x4641,
		      SPOTLIT_SEE_CONE		= 0x4650,
		      SPOTLIT_RECTANGULAR	= 0x4651,
		      SPOTLIT_ASPECT		= 0x4657,
		      SPOTLIT_PROJECTOR		= 0x4653,
		      SPOTLIT_OVERSHOOT		= 0x4652,
		      SPOTLIT_RAY_BIAS		= 0x4658,
		      SPOTLIT_RAYSHAD		= 0x4627,
		  OBJ_CAMERA				= 0x4700,
		    // OBJ_CAMERA
		    CAM_SEE_CONE 			= 0x4710, 
		    CAM_RANGES 				= 0x4720, 
		  
		  OBJ_HIDDEN				= 0x4010,
		  OBJ_VIS_LOFTER			= 0x4011,
		  OBJ_DOESNT_CAST			= 0x4012,
		  OBJ_MATTE					= 0x4013,
		  OBJ_FAST					= 0x4014,
		  OBJ_PROCEDURAL			= 0x4015,
		  OBJ_FROZEN				= 0x4016,
		  OBJ_DONT_RCVSHADOW		= 0x4017,

		EDIT_MATERIAL 			= 0xAFFF,	//!< �ގ�
		  // EDIT_MATERIAL
		  MAT_NAME				= 0xA000, 
		  MAT_AMBIENT			= 0xA010, 
		  MAT_DIFFUSE			= 0xA020, 
		  MAT_SPECULAR			= 0xA030, 
		  MAT_SHININESS			= 0xA040, 
		  MAT_SHIN2PCT			= 0xA041, 
		  MAT_TRANSPARENCY		= 0xA050, 
		  MAT_XPFALL			= 0xA052, 
		  MAT_SELF_ILPCT		= 0xA084, 
		  MAT_USE_XPFALL		= 0xA240, 
		  MAT_REFBLUR			= 0xA053, 
		  MAT_USE_REFBLUR		= 0xA250, 
		  MAT_SHADING			= 0xA100, 
		  MAT_SELF_ILLUM		= 0xA080, 
		  MAT_TWO_SIDE			= 0xA081, 
		  MAT_DECAL				= 0xA082, 
		  MAT_ADDITIVE			= 0xA083, 
		  MAT_FACEMAP			= 0xA088, 
		  MAT_PHONGSOFT			= 0xA08C, 
		  MAT_WIRE				= 0xA085, 
		  MAT_WIREABS			= 0xA08E, 
		  MAT_WIRE_SIZE			= 0xA087, 
		  
		  MAT_ACUBIC			= 0xA310, 
		  
		  MAT_TEX_MAP			= 0xA200, 
		  MAT_TEX_MASK			= 0xA33E, 
		  MAT_TEX2_MAP			= 0xA33A, 
		  MAT_TEX2_MASK			= 0xA340, 
		  MAT_OPAC_MAP			= 0xA210, 
		  MAT_OPAC_MASK			= 0xA342, 
		  MAT_BUMP_MAP			= 0xA230, 
		  MAT_BUMP_MASK			= 0xA344, 
		  MAT_SPEC_MAP			= 0xA204, 
		  MAT_SPEC_MASK			= 0xA348, 
		  MAT_SHIN_MAP			= 0xA33C, 
		  MAT_SHIN_MASK			= 0xA346, 
		  MAT_SELFI_MAP			= 0xA33D, 
		  MAT_SELFI_MASK		= 0xA34A, 
		  MAT_REFL_MAP			= 0xA220, 
		  MAT_REFL_MASK			= 0xA34C, 
		    // MAT_*_MAP, MAT_*_MASK
		    MAP_FILENAME		= 0xA300, 
		    MAP_TILING			= 0xA351, 
		    MAP_BLUR			= 0xA353, 
		    MAP_SCALE_U			= 0xA354, 
		    MAP_SCALE_V			= 0xA356, 
		    MAP_OFFSET_U		= 0xA358, 
		    MAP_OFFSET_V		= 0xA35A, 
		    MAP_ROTATION		= 0xA35C, 
		    MAP_COL1			= 0xA360, 
		    MAP_COL2			= 0xA362, 
		    MAP_RCOL			= 0xA364, 
		    MAP_GCOL			= 0xA366, 
		    MAP_BCOL			= 0xA368, 


		// KEYF3DS
		KEYF_CURRENT_TIME		= 0xB009,
		KEYF_HDR				= 0xB00A,
		KEYF_FRAMES				= 0xB008,
		KEYF_AMBIENT_NODE_TAG	= 0xB001,
		KEYF_OBJECT_NODE_TAG	= 0xB002,
		KEYF_CAMERA_NODE_TAG	= 0xB003,
		KEYF_TARGET_NODE_TAG	= 0xB004,
		KEYF_LIGHT_NODE_TAG		= 0xB005,
		KEYF_L_TARGET_NODE_TAG	= 0xB006,
		KEYF_SPOTLIGHT_NODE_TAG	= 0xB007,


		// color chunks
		COL_RGB 				= 0x0010,
		COL_TRU 				= 0x0011,
		COL_LIN_RGB				= 0x0012,
		COL_LIN_TRU				= 0x0013,
		INT_PERCENTAGE			= 0x0030,
		FLOAT_PERCENTAGE		= 0x0031,
	};

	enum rxObjFlag
	{
		OBJ_FLAG_HIDDEN			= 0x01, 
		OBJ_FLAG_VIS_LOFTER		= 0x02, 
		OBJ_FLAG_DOESNT_CAST	= 0x04, 
		OBJ_FLAG_MATTE			= 0x08, 
		OBJ_FLAG_FAST			= 0x10, 
		OBJ_FLAG_PROCEDURAL		= 0x20, 
		OBJ_FLAG_FROZEN			= 0x40, 
		OBJ_FLAG_DONT_RCVSHADOW	= 0x80, 
	};

	enum rxDefaultView
	{
		DEFAULT_VIEW_TOP = 1,
		DEFAULT_VIEW_BOTTOM,
		DEFAULT_VIEW_LEFT,
		DEFAULT_VIEW_RIGHT,
		DEFAULT_VIEW_FRONT,
		DEFAULT_VIEW_BACK,
		DEFAULT_VIEW_USER,
		DEFAULT_VIEW_CAMERA,
	};

protected:
	//
	// �`�����N�ǂݍ��݊֐�
	//
	//! EDIT3DS�`�����N�̓ǂݍ���
	int readEditChunk(ifstream &file, vector<Vec3> &vrts, vector<rxFace> &plys);

	//! KEYF3DS�`�����N�̓ǂݍ���
	int readKeyframeChunk(ifstream &file);

	//! �I�u�W�F�N�g�`�����N�̓ǂݍ���
	int readObjectChunk(ifstream &file, vector<Vec3> &vrts, vector<rxFace> &plys);
	int readTriMeshChunk(ifstream &file, vector<Vec3> &vrts, vector<rxFace> &plys);
	int readBackgroundChunk(ifstream &file);

	//! �r���[�|�[�g�`�����N�̓ǂݍ���
	int readDefaultViewChunk(ifstream &file, rxViewport3DS &viewport);
	int readViewportChunk(ifstream &file, rxViewport3DS &viewport);

	//! �O�p�`���b�V���`�����N�̓ǂݍ���
	int readVertexChunk(ifstream &file, vector<Vec3> &vrts);
	int readVertexFlagChunk(ifstream &file, vector<int> &vrt_flags);
	int readFaceChunk(ifstream &file, vector<rxFace> &plys, rxMesh3DS &mesh);
	int readFaceMaterialChunk(ifstream &file, vector<rxFace> &plys);
	int readTexCoordChunk(ifstream &file, vector<Vec2> &tc);
	int readSmoothingChunk(ifstream &file, int n);
	int readTranslationChunk(ifstream &file, float mat[4][4]);
	int readMeshTextureInfoChunk(ifstream &file, rxMesh3DS &mesh);
	
	//! �����`�����N�̓ǂݍ���
	int readLightChunk(ifstream &file, rxLight3DS &light);
	int readSpotChunk(ifstream &file, rxLight3DS &light);

	//! �J�����`�����N�̓ǂݍ���
	int readCameraChunk(ifstream &file, rxCamera3DS &camera);

	//! �ގ��`�����N�̓ǂݍ���
	int readMaterialChunk(ifstream &file, vector<rxMaterial3DS> &mats);
	int readMatNameChunk(ifstream &file, string &name);
	int readMapChunk(ifstream &file, rxMap3DS &map);

	//! �t�H�O�`�����N�̓ǂݍ���
	int readFogChunk(ifstream &file);
	int readFogLayerChunk(ifstream &file);
	int readFogDistanceCueChunk(ifstream &file);


	//! ���̑��̃`�����N
	int readUnknownChunk(ifstream &file);
	int readBooleanChunk(ifstream &file, bool &b);
	int readIntChunk(ifstream &file, int &x);
	int readFloatChunk(ifstream &file, float &x);
	int readIntPercentageChunk(ifstream &file, float &x);
	int readColorChunk(ifstream &file, Vec3 &color);
	int readColorChunk(ifstream &file, Vec4 &color);

};



#endif // _RX_OBJ_H_
