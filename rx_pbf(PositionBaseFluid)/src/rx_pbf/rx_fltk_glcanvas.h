/*!
  @file rx_fltk_glcanvas.h
	
  @brief FLTK�ɂ��OpenGL�E�B���h�E�N���X
 
  @author Makoto Fujisawa 
  @date   2011-09
*/

#ifndef _RX_FLTK_GLCANVAS_H_
#define _RX_FLTK_GLCANVAS_H_



//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
#include <iostream>

// STL
#include <vector>
#include <string>

// FLTK
#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Check_Button.H>

#include "rx_sph_commons.h"
#include "rx_sph_config.h"
#include "rx_fltk_widgets.h"

#include "rx_trackball.h"
#include "rx_model.h"

#include "rx_texture.h"


using namespace std;

//-----------------------------------------------------------------------------
// ��`/�萔
//-----------------------------------------------------------------------------
class rxFlWindow;
class rxParticleSystemBase;
struct rxCell;

class rxMCMeshCPU;
class rxMCMeshGPU;

#define RX_USE_GPU


#if defined(RX_USE_GPU)
	#define RXSPH rxPBDSPH_GPU
#else
	#define RXSPH rxPBDSPH
#endif



// �`��t���O
enum
{
	RXD_PARTICLE		= 0x0001,	//!< �p�[�e�B�N��
	RXD_VELOCITY		= 0x0002,	//!< ���x��
	RXD_BBOX			= 0x0004,	//!< AABB(�V�~�����[�V�������)
	RXD_AXIS			= 0x0008,   //!< ��

	RXD_CELLS			= 0x0010,	//!< �ߖT�T���p�����Z��
	RXD_MESH			= 0x0020,	//!< ���b�V��
	RXD_SOLID			= 0x0040,	//!< �ő�
	RXD_REFRAC			= 0x0080,	//!< ���ܕ`��

	RXD_BPARTICLE		= 0x0100,	//!< ���E�p�[�e�B�N��
	RXD_BPARTICLE_ENV	= 0x0200,	//!< ���E�p�[�e�B�N���`��
	RXD_ZPLANE_CLIP		= 0x0400,	//!< �f��
	RXD_PARAMS			= 0x0800,	//!< �p�����[�^��ʕ`��
};
const string RX_DRAW_STR[] = {
	"Particle",					"p", 
	"Velocity",					"v", 
	"AABB (Simulation Space)", 	"b",
	"Axis",				 		"",

	"Cells", 					"d",
	"Mesh", 					"m",
	"Solid", 					"o",
	"Refrac", 					"r",
	
	"Boundary Particle",		"B",
	"Env. Boundary Prt.",		"", 
	"Cross Section",			"",
	"Params", 					"P",
	"-1"
};

// �p�[�e�B�N���`����@
enum
{
	RXP_POINTSPRITE = 0, 
	RXP_POINT, 
	RXP_SPHERE, 
	RXP_POINT_NONE, 

	RXP_END, 
};
const string RX_PARTICLE_DRAW[] = {
	"Point Sprite",		"^1", 
	"GL_POINT", 		"^2", 
	"glutSolidSphere", 	"^3", 
	"None", 			"^4", 
	"-1"
};

// �p�[�e�B�N���`��F���@
enum
{
	RXP_COLOR_CONSTANT = 0, 
	RXP_COLOR_DENSITY, 
	RXP_COLOR_RAMP, 
	RXP_COLOR_END, 
};
const string RX_PARTICLE_COLOR[] = {
	"Constant", 	"", 
	"Density", 		"", 
	"Ramp",			"", 
	"-1"
};


// �ő̕`��
enum
{
	RXS_VERTEX		= 0x0001, 
	RXS_EDGE		= 0x0002, 
	RXS_FACE		= 0x0004, 
	RXS_NORMAL		= 0x0008, 
	RXS_END
};
const string RXS_STR[] = {
	"Vertex", "", 
	"Edge",   "", 
	"Face",   "", 
	"Normal", "", 
	"-1"
};

// �t�̕\�ʕ`��
enum
{
	RXL_VERTEX		= 0x0001, 
	RXL_EDGE		= 0x0002, 
	RXL_FACE		= 0x0004, 
	RXL_NORMAL		= 0x0008, 
	RXL_END
};
const string RXL_STR[] = {
	"Vertex", "", 
	"Edge",   "", 
	"Face",   "", 
	"Normal", "", 
	"-1"
};


//! �`��̈�T�C�Y���
const string RX_CANVAS_SIZE_STR[] = {
	"1920x1080",	"",
	"1280x720",		"", 
	"1024x768",		"", 
	"800x800",		"", 
	"800x600",		"", 
	"640x480",		"", 
	"-1", 
};


//! SPH�ݒ�
enum SettingMode
{
	RX_SPH_MESH			= 0x0001,	// ���b�V������
	RX_SPH_INPUT		= 0x0002,		// �p�[�e�B�N���f�[�^�o��
	RX_SPH_OUTPUT		= 0x0004,		// �p�[�e�B�N���f�[�^����
	RX_SPH_MESH_OUTPUT	= 0x0008, // ���b�V���o��
	RX_SPH_INLET		= 0x0010,		// �������E

	RX_SPH_END			= 0x0040, 
};

//! �s�b�N���ꂽ�I�u�W�F�N�g�Ɋւ�����
struct rxPickInfo
{
	GLuint name;		//!< �q�b�g�����I�u�W�F�N�g�̖��O
	float min_depth;	//!< �v���~�e�B�u�̃f�v�X�l�̍ŏ��l
	float max_depth;	//!< �v���~�e�B�u�̃f�v�X�l�̍ő�l
};

static inline bool CompFuncPickInfo(rxPickInfo a, rxPickInfo b)
{
	return a.min_depth < b.min_depth;
}

//-----------------------------------------------------------------------------
//! rxFlGLWindow�N���X - fltk�ɂ��OpenGL�E�B���h�E
//-----------------------------------------------------------------------------
class rxFlGLWindow : public Fl_Gl_Window
{
protected:
	int m_iWinW;					//!< �`��E�B���h�E�̕�
	int m_iWinH;					//!< �`��E�B���h�E�̍���
	int m_iMouseButton;				//!< �}�E�X�{�^���̏��
	int m_iKeyMod;					//!< �C���L�[�̏��
	rxTrackball m_tbView;			//!< �g���b�N�{�[��

	double m_fBGColor[3];			//!< �w�i�F
	bool m_bAnimation;				//!< �A�j���[�V����ON/OFF
	bool m_bFullscreen;				//!< �t���X�N���[��ON/OFF

	rxFlWindow *m_pParent;			//!< �e�N���X

	vector<rxPolygons> m_vPolys;	//!< �|���S���I�u�W�F�N�g

	// FTGL
	unsigned long m_ulFontSize;		//!< �t�H���g�T�C�Y

	//
	// ���q�@�֘A�ϐ�
	//
	rxParticleSystemBase *m_pPS;	//!< SPH
	double m_fDt;					//!< �^�C���X�e�b�v��
	double m_fGravity;				//!< �d�͉����x

	int m_iCurrentStep;				//!< ���݂̃X�e�b�v��
	bool m_bPause;					//!< �V�~�����[�V�����̃|�[�Y�t���O

	// �V�[��
	rxSceneConfig m_Scene;


	// �p�[�e�B�N�����o��
	string m_strSphOutputName0 ;
	string m_strSphOutputHeader;

	// �p�[�e�B�N��������
	string m_strSphInputName0 ;
	string m_strSphInputHeader;


	//
	// ���b�V��
	//
	uint m_iNumVrts, m_iNumTris;	//!< �������ꂽ���b�V���̒��_���ƃ��b�V����
	int m_iVertexStore;				//!< �T���v�����O�{�����[�����ɑ΂���\�z����钸�_��(nx*ny*store)

	int m_iMeshMaxN;				//!< ���b�V�����O���b�h��(���E�������Ƃ������������̕�����)
	int m_iMeshN[3];				//!< ���b�V�����O���b�h��

	Vec3 m_vMeshBoundaryExt;		//!< ���b�V�����E�{�b�N�X�̊e�ӂ̒�����1/2
	Vec3 m_vMeshBoundaryCen;		//!< ���b�V�����E�{�b�N�X�̒��S���W

	rxPolygons m_Poly;				//!< ���b�V��
	rxMaterialOBJ m_matPoly;

	GLuint m_uVrtVBO;				//!< ���b�V�����_(VBO)
	GLuint m_uTriVBO;				//!< ���b�V���|���S��(VBO)
	GLuint m_uNrmVBO;				//!< ���b�V�����_�@��(VBO)
	int m_iDimVBO;

	// ���b�V���o��
	int m_iSaveMeshSpacing;

	// �w�i�摜
	bool m_bUseCubeMap;				//!< �L���[�u�}�b�v�g�p�t���O
	rxCubeMapData m_CubeMap;		//!< �L���[�u�}�b�v

	// ���b�V������
	rxMCMeshCPU *m_pMCMeshCPU;
	rxMCMeshGPU *m_pMCMeshGPU;


	int m_iPickedParticle;			//!< �}�E�X�s�b�N���ꂽ�p�[�e�B�N��
	int m_iSelBufferSize;			//!< �Z���N�V�����o�b�t�@�̃T�C�Y
	GLuint* m_pSelBuffer;			//!< �Z���N�V�����o�b�t�@


public:
	// �`��t���O
	int m_iDraw;					//!< �`��t���O
	int m_iDrawPS;					//!< �p�[�e�B�N���`����@
	int m_iColorType;				//!< �p�[�e�B�N���`�掞�̐F
	int m_iSolidDraw;				//!< �ő̕`��
	int m_iLiquidDraw;				//!< �t�̕\�ʕ`��

	// �V�~�����[�V�����ݒ�
	int m_iSimuSetting;		//!< �~�����[�V�����ݒ�t���O
	double m_fVScale;				//!< �x�N�g����`�掞�̃X�P�[��
	double m_fMeshThr;				//!< �A�֐����b�V��������臒l

	double m_fClipPlane[2];			//!< �N���b�v����(z���ɐ����ȕ���)

	// �V�[�����X�g
	vector<string> m_vSceneTitles;	//!< �V�[���t�@�C�����X�g
	int m_iCurrentSceneIdx;			//!< ���݂̃V�[���t�@�C��

	// �摜�o��
	int m_iSaveImageSpacing;		//!< �摜�ۑ��Ԋu(=-1�Ȃ�ۑ����Ȃ�)

public:
	//! �R���X�g���N�^
	rxFlGLWindow(int x, int y, int w, int h, const char* l, void *parent);

	//! �f�X�g���N�^
	~rxFlGLWindow();

public:
	// OpenGL������
	void InitGL(void);

	// OpenGL�`��
	void Projection(void);
	vector<string> SetDrawString(void);
	void ReDisplay(void);

	// GUI�R�[���o�b�N
	void Display(void);
	void Resize(int w, int h);
	void Mouse(int button, int state, int x, int y);
	void Motion(int x, int y);
	void PassiveMotion(int x, int y);
	void Idle(void);
	void Timer(void);
	void Keyboard(int key, int x, int y);
	void SpecialKey(int key, int x, int y);

	// �}�E�X�s�b�N�p
	static void Projection_s(void* x);
	static void DisplayForPick_s(void* x);
	void DisplayForPick(void);
	bool PickParticle(int x, int y);

	// ���_
	void InitView(void);

	// �A�j���[�V����
	static void OnTimer_s(void* x);
	static void OnIdle_s(void* x);
	
	// �A�j���[�V�����؂�ւ�
	bool SwitchIdle(int on);

	// �t���X�N���[���؂�ւ�
	void SwitchFullScreen(int win = 1);
	int  IsFullScreen(void);


	// �t�@�C�����o��
	void OpenFile(const string &fn);
	void SaveFile(const string &fn);
	void SaveDisplay(const string &fn);
	void SaveDisplay(const int &stp);
	
	void SaveMesh(const string fn, rxPolygons &polys);
	void SaveMesh(const int &stp, rxPolygons &polys);


public:
	// ���b�V������
	bool CalMeshSPH(int nmax, double thr = 1000.0);
	bool ResetMesh(void);
	void SetMeshCreation(void);
	RXREAL GetImplicitSPH(double x, double y, double z);

protected:
	bool calMeshSPH_CPU(int nmax, double thr = 1000.0);
	bool calMeshSPH_GPU(int nmax, double thr = 1000.0);
	
	// �}�E�X�s�b�N
	vector<rxPickInfo> selectHits(GLint nhits, GLuint buf[]);
	vector<rxPickInfo> pick(int x, int y, int w, int h);

public:
	// SPH
	void InitSPH(rxSceneConfig &sph_scene);
	void InitSPH(void){ InitSPH(m_Scene); }
	void AddSphere(void);

	void StepPS(double dt);
	void ComputeFPS(void);

	void DivideString(const string &org, vector<string> &div);

	void DrawParticleVector(RXREAL *prts, RXREAL *vels, int n, int d, double *c0, double *c1, double len = 0.1);
	void DrawParticlePoints(unsigned int vbo, int n, unsigned int color_vbo = 0, RXREAL *data = 0, int offset = 0);

	void CreateVBO(GLuint* vbo, unsigned int size);
	void DeleteVBO(GLuint* vbo);

	void DrawLiquidSurface(void);
	void SetParticleColorType(int type);

	void RenderSphScene(void);

	bool IsArtificialPressureOn(void);

private:
	//! �`��R�[���o�b�N�֐��̃I�[�o�[���C�h
	void draw(void)
	{
		if(!context_valid()) InitGL();
		if(!valid()) Resize(w(), h());
		Display();    // OpenGL�`��
	}

	//! ���T�C�Y�R�[���o�b�N�֐��̃I�[�o�[���C�h
	void resize(int x_, int y_, int w_, int h_)
	{
		Fl_Gl_Window::resize(x_, y_, w_, h_);
		//Resize(w_, h_);
	}


public:
	// �C�x���g�n���h��
	int handle(int e);	// handle�֐��̃I�[�o�[���C�h

	// ���j���[�C�x���g�n���h��
	static void OnMenuDraw_s(Fl_Widget *widget, void* x);
	inline void OnMenuDraw(double val, string label);

	static void OnMenuSimulation_s(Fl_Widget *widget, void* x);
	inline void OnMenuSimulation(double val, string label);

	static void OnMenuParticle_s(Fl_Widget *widget, void* x);
	inline void OnMenuParticle(double val, string label);
	inline void OnMenuParticleColor(double val, string label);
	inline void OnMenuParticleDraw(double val, string label);

	static void OnMenuMesh_s(Fl_Widget *widget, void* x);
	inline void OnMenuMesh(double val, string label);
	inline void OnMenuMeshSolid(double val, string label);
	inline void OnMenuMeshLiquid(double val, string label);

	static void OnMenuScene_s(Fl_Widget *widget, void* x);
	inline void OnMenuScene(double val, string label);
};




#endif // #ifdef _RX_FLTK_GLCANVAS_H_
