/*!
  @file rx_fltk_glcanvas.h
	
  @brief FLTK�ɂ��OpenGL�E�B���h�E�N���X
 
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
#include "rx_pov.h"

#include "rx_texture.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>


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

#ifdef RX_USE_GPU
	#define RXSPH rxSPH_GPU
#else
	#define RXSPH rxSPH
#endif


// �`��t���O
enum
{
	RXD_PARTICLE		= 0x0001,	//!< �p�[�e�B�N��
	RXD_VELOCITY		= 0x0002,	//!< ���x��
	RXD_NORMAL			= 0x0004,	//!< �@��
	RXD_BBOX			= 0x0008,	//!< AABB(�V�~�����[�V�������)
	RXD_CELLS			= 0x0010,	//!< �ߖT�T���p�����Z��
	RXD_MESH			= 0x0020,	//!< ���b�V��
	RXD_SOLID			= 0x0040,	//!< �ő�
	RXD_REFRAC			= 0x0080,	//!< ���ܕ`��

	RXD_PARAMS			= 0x0100,	//!< �p�����[�^��ʕ`��
	RXD_ANISOTROPICS	= 0x0200,	//!< �ٕ����J�[�l��
};
const string RX_DRAW_STR[] = {
	"Particle",					"p", 
	"Velocity",					"v", 
	"Normal",					"n",
	"AABB (Simulation Space)", 	"b",
	"Cells", 					"d",
	"Mesh", 					"m",
	"Solid", 					"o",
	"Refrac", 					"r",
	"Params", 					"P",
	"Anisotoropics", 			"A",
	"-1"
};

// �p�[�e�B�N���`����@
enum
{
	RXP_POINTSPRITE = 0, 
	RXP_POINT, 
	RXP_POINT_NONE, 

	RXP_END, 
};
const string RX_PARTICLE_DRAW[] = {
	"Point Sprite", "^1", 
	"GL_POINT", 	"^2", 
	"None", 		"^3", 
	"-1"
};


// �O�p�`���b�V�������@
enum
{
	RXM_MC_CPU = 0, 
	RXM_MC_GPU, 
};
const string RX_TRIANGULATION_METHOD[] = {
	"Marching Cubes (CPU)",	   "", 
	"Marching Cubes (GPU)",    "", 
	"-1"
};

// �ő̕`��
enum
{
	RXS_VERTEX		= 0x0001, 
	RXS_EDGE		= 0x0002, 
	RXS_FACE		= 0x0004, 
	RXS_NORMAL		= 0x0008, 
	RXS_MOVE		= 0x0010,

	RXS_END
};
const string RXS_STR[] = {
	"Vertex", "", 
	"Edge",   "", 
	"Face",   "", 
	"Normal", "", 
	"Move",   "", 
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
	ID_SPH_MESH = 0,	// ���b�V������
	ID_SPH_INPUT,		// �p�[�e�B�N���f�[�^�o��
	ID_SPH_OUTPUT,		// �p�[�e�B�N���f�[�^����
	ID_SPH_MESH_OUTPUT, // ���b�V���o��
	ID_SPH_INLET,		// �������E
	ID_SPH_VC, 
	ID_SPH_ANISOTROPIC, // �ٕ����J�[�l��

	ID_SPH_END, 
};


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
	//string m_strCurrentScene;		//!< ���݂̃V�[���̖��O
	//vector<string> m_vSceneFiles;	//!< �V�[���t�@�C�����X�g
	//int m_iSceneFileNum;			//!< �V�[���t�@�C���̐�
	rxSPHConfig m_Scene;

	int m_iSimuSetting;				//!< �~�����[�V�����ݒ�ۑ��p

	// �ő̈ړ��t���O
	bool m_bSolidMove;

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
	//vector<rxPolygons*> m_vSolidPoly;//!< �ő̃��b�V��
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


public:
	// �`��t���O
	int m_iDraw;					//!< �`��t���O
	int m_iDrawPS;					//!< �p�[�e�B�N���`����@
	int m_iColorType;				//!< �p�[�e�B�N���`�掞�̐F
	int m_iTriangulationMethod;		//!< �O�p�`���b�V�������@
	int m_iSolidDraw;				//!< �ő̕`��

	// �V�~�����[�V�����ݒ�
	bitset<32> m_bsSimuSetting;		//!< �~�����[�V�����ݒ�t���O
	double m_fVScale;				//!< �x�N�g����`�掞�̃X�P�[��
	double m_fMeshThr;				//!< �A�֐����b�V��������臒l

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

	// FTGL�t�H���g�ݒ�
	int SetupFonts(const char* file);


public:
	// ���b�V������
	bool CalMeshSPH(int nmax, double thr = 1000.0);
	bool ResetMesh(void);
	void SetMeshCreation(void);
	RXREAL GetImplicitSPH(double x, double y, double z);

protected:
	bool calMeshSPH_CPU(int nmax, double thr = 1000.0);
	bool calMeshSPH_GPU(int nmax, double thr = 1000.0);

	// SPH
	void InitSPH(rxSPHConfig &sph_scene);
	void InitSPH(void){ InitSPH(m_Scene); }
	void AddSphere(void);

	void StepPS(double dt);
	void ComputeFPS(void);

	void DivideString(const string &org, vector<string> &div);

	void DrawParticleVector(RXREAL *prts, RXREAL *vels, int n, int d, double *c0, double *c1, double len = 0.1);
	void DrawParticlePoints(unsigned int vbo, int n, unsigned int color_vbo = 0, RXREAL *data = 0);
	void DrawSubParticles(void);

	void CreateVBO(GLuint* vbo, unsigned int size);
	void DeleteVBO(GLuint* vbo);

	void DrawLiquidSurface(void);
	void SetParticleColorType(int type, int change = 0);

	void RenderSphScene(void);
	

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

	static void OnMenuSolid_s(Fl_Widget *widget, void* x);
	inline void OnMenuSolid(double val, string label);

	static void OnMenuTriangulation_s(Fl_Widget *widget, void* x);
	inline void OnMenuTriangulation(double val, string label);

	static void OnMenuScene_s(Fl_Widget *widget, void* x);
	inline void OnMenuScene(double val, string label);
};




#endif // #ifdef _RX_FLTK_GLCANVAS_H_
