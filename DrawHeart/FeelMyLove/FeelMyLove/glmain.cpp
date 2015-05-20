/*
@fiel glmain.cpp

@brief basic structure of GLUT 

@date 2015-05
*/

#pragma comment (lib,"glew32.lib")

//include files
//--------------------------------------------------------------------------------

#include "heart.h"
#include "rx_trackball.h"
#include <cmath>
//define program name
const string RX_PROGRAM_NAME = "Feel My Love";

// drawing flag
enum
{
	RXD_VERTEX		= 0x0001,	//!< ���_�`��
	RXD_EDGE		= 0x0002,	//!< �G�b�W�`��
	RXD_FACE		= 0x0004,	//!< �ʕ`��
	RXD_NORMAL		= 0x0008,	//!< �@���`��
};
const string RX_DRAW_STR[] = {
	"Vertex",	"v", 
	"Edge",		"e", 
	"Face",		"f", 
	"Normal",	"n", 
	"-1"
};

//windows size
int g_iWinW = 720;		//Width of the window
int g_iWinH = 720;		//Height of the window
int g_iWinX = 100;		//x position of the window
int g_iWinY = 100;		//y position of the window

bool g_bIdle = false;		//state of the idle
int g_iDraw = RXD_FACE;	//!< drawing flag

double g_fAng = 0.0;	//!< �`���]�p

//background color
double g_fBGColor[4] = {0.8,0.9,1.0,1.0};

//rxtrackball,to control the viewpoint
rxTrackball g_tbView;

//function declaration & definition 
void OnExitApp(int id =-1);
void Idle(void);
void CleanGL(void);
void drawHeart(float size );

//control procedure
/*
idle cantrol,in which, ON is true,false is OFF
*/
void SwitchIdle(int on)
{
	g_bIdle = (on==-1)?!g_bIdle:(on?true:false);
	glutIdleFunc((g_bIdle?Idle:0));
	cout<<"idle"<<(g_bIdle?"on":"off")<<endl;
}

//display on fullscreen / window
void SwitchFullScreen(void)
{
	static int fullscreen =0;
	static int pos0[2] = {0,0};
	static int win0[2] = {500,500};
	if(fullscreen){
		glutPositionWindow(pos0[0],pos0[1]);
		glutReshapeWindow(win0[0],win0[1]);
	}
	else{
		pos0[0] = glutGet(GLUT_WINDOW_X);
		pos0[1] = glutGet(GLUT_WINDOW_Y);
		win0[0] = glutGet(GLUT_WINDOW_WIDTH);
		win0[1] = glutGet(GLUT_WINDOW_HEIGHT);

		glutFullScreen();
	}
	fullscreen ^=1;
}

//Exit the application
void OnExitApp(int id)
{
	CleanGL();
	exit(1);
}

//display function

void Projection(void)
{
	gluPerspective(45.0f,(float)g_iWinW/(float)g_iWinH,0.2f,1000.0f);
}

void RenderScene(void)
{
	glPushMatrix();

	glEnable(GL_LIGHTING);
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

	glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	//draw object------------------------------------------------------------

//	glutSolidTeapot(1.0f);		//we draw a teapot here
	
	int n = 200;
	float r = 0.5;
	float pi = 3.141593;
	/*
	glBegin(GL_LINES);
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			float u1 = j * 2 * pi / n;
			float u2 = (j + 1) * 2 * pi / n;
			float v = i * 2 * pi / n;
			float x1 = r * (cos(v + pi / 2) + 2 * sin(v / 2) * cos(u1));
			float y1 = r * (sin(v + pi / 2) + 2 * sin(v / 2) * sin(u1));
			float x2 = r * (cos(v + pi / 2) + 2 * sin(v / 2) * cos(u2));
			float y2 = r * (sin(v + pi / 2) + 2 * sin(v / 2) * sin(u2));
			glVertex2f(x1, y1);
			glVertex2f(x2, y2);
		}
	}
	glEnd();
	*/
for(float size =1; size <=4;size +=0.2f)
{glColor3d(1.0f/size,0.0f,0.0f);
	drawHeart(size);
}

	glPopMatrix();
}

void drawHeart(float size )
{	 glTranslatef(0, 0.0f, -0.2*(0.2-1/size));
		glBegin(GL_POLYGON);
        for (float x = -1.139; x <= 1.139; x += 0.001) 
        {
            float delta = pow((x*x), 1.0 / 3.0) * pow((x*x), 1.0 / 3.0)  - 4*x*x + 4;
            float y1 = (pow((x*x), 1.0 / 3.0)  + sqrt(delta)) / 2;
            float y2 = (pow((x*x), 1.0 / 3.0)  - sqrt(delta)) / 2;
            glVertex2f(x/size, y1/size);
            glVertex2f(x/size, y2/size);
        }
    glEnd();
}
//envent handle
void Display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();

	g_tbView.Apply();		//mouse control, which allows to rotate and move

	glPushMatrix();
	glRotated(g_fAng,0,1,0);
	RenderScene();
	glPopMatrix();

	glPopMatrix();

	//word 
	vector<string> strs;
	strs.push_back("\"s\" key : idle on/off");
	strs.push_back("SHIFT+\"f\" key : fullscreen on/off" );
	strs.push_back("\"v\",\"e\",\"f\" key : switch vertex,edge,face drawing");
	glColor3d(1.0,1.0,1.0);
	//drawString();                 //draw the string view of the teapot

	glutSwapBuffers();
}

//idle handle
void Idle(void)
{
	g_fAng +=0.5f;
	glutPostRedisplay();

}

//init the GL
void InitGL(void)
{
	cout <<"OpenGL ver." << glGetString(GL_VERSION) << endl;

	GLenum err = glewInit();
	if(err == GLEW_OK){
		cout<<"GLEW OK : GLew ver. "<< glewGetString(GLEW_VERSION)<<endl;
	}
	else{
		cout << "GLEW Error :"<<glewGetErrorString(err) << endl;
	}

	//multisample
	GLint buf,sbuf;
	glGetIntegerv(GL_SAMPLER_BUFFER,&buf);
	cout<<"number of sample buffers is "<<buf<<endl;
	glGetIntegerv(GL_SAMPLES,&sbuf);
	cout<<"number of samples is " <<sbuf << endl;

	glClearColor((GLfloat)g_fBGColor[0],(GLfloat)g_fBGColor[1],(GLfloat)g_fBGColor[2],1.0f);
	glClearDepth(1.0f);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glEnable(GL_MULTISAMPLE);

	//nomalize the polygon
	glEnable(GL_AUTO_NORMAL);
	glEnable(GL_NORMALIZE);

	//set up the light
	static const GLfloat lpos[] = {0.0f,0.0f,1.0f,1.0f};//position of the light
	static const GLfloat difw[] = {0.7f,0.7f,0.7f,1.0f};//diffuse
	static const GLfloat spec[] = {0.2f,0.2f,0.2f,1.0f};//specular
	static const GLfloat ambi[] = {0.1f,0.1f,0.1f,1.0f};//ambient
	glLightfv(GL_LIGHT0,GL_POSITION,lpos);
	glLightfv(GL_LIGHT0,GL_DIFFUSE,difw);
	glLightfv(GL_LIGHT0,GL_SPECULAR,spec);
	glLightfv(GL_LIGHT0,GL_AMBIENT,ambi);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHTING);

	glShadeModel(GL_SMOOTH);

	//init the trackball
	g_tbView.SetScaling(-5.0f);
}

//resize the window
void Resize(int w,int h)
{
	g_iWinW =w;
	g_iWinH = h;
	glViewport(0,0,w,h);
	g_tbView.SetRegion(w,h);

	//set the Parameters of the projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	Projection();

	//set up hte modelview
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

/*
set the mouse control button
@param[in] button: mouse button(GLUT_LEFT_BUTTON,GLUT_MIDDLE_BUTTON.GLUT_RIGHT_BUTTON)
@param[in] state: the state of the mouse button(GLUT_UP.GLUT_DOWN)
@param[in] x,y the positon of the mouse button

*/
void Mouse(int button,int state,int x,int y)
{
	if(x<0||y<0) return;

	int mod = glutGetModifiers();		//get the state mode of SHIFT,CTRL,ALT

	if(button == GLUT_LEFT_BUTTON){
		if(state == GLUT_DOWN)
		{
			g_tbView.Start(x,y,mod+1);
		}
		else if(state == GLUT_UP)
		{
			g_tbView.Stop(x,y);
		}
	}
	else if (button == GLUT_MIDDLE_BUTTON){
	}
	else if(button == GLUT_RIGHT_BUTTON)
	{
	}
	glutPostRedisplay();
}

//motion function(put down the button)
//@param[in] x,y :the positon of the mouse
void Motion(int x,int y)
{
	if(x<0 || y<0) return;
	int mod = glutGetModifiers();		//get the state mode of Shift,Ctrl,Alt
	g_tbView.Motion(x,y);
	glutPostRedisplay();
}

//motion function()
void PassiveMotion(int x,int y)
{
	if(x<0 ||y<0) return;
}

/*keybord function
@param[in] key, the type of the key
@param[in] x,y : the current position of the mouse
*/
void Keyboard(unsigned char key, int x, int y)
{
	int mod = glutGetModifiers();		//the state(Shift,Ctrl,Alt)

	switch(key){
	case '\033': // the ASCII code of ESC
		OnExitApp();
		break;

	case ' ':	//idle
		Idle();
		break;

	case 's':	//animation ON/OFF
		SwitchIdle(-1);
		break;

	case 'F':   //Fullscreen
		SwitchFullScreen();
		break;

	default:	//drawing flag
		{
			int idx = 0;
			while(RX_DRAW_STR[2*idx]!="-1"&&(int)RX_DRAW_STR[2*idx+1][0]!=key) idx++;
			if(RX_DRAW_STR[2*idx]!="-1"){
				g_iDraw=(0x01<<idx);
			}
		}
		break;
	}
	glutPostRedisplay();
}

//special key
void SpecialKey(int key,int x,int y)
{
	switch(key){
	case GLUT_KEY_LEFT:
		break;

	case GLUT_KEY_RIGHT:
		break;

	case GLUT_KEY_UP:
		break;

	case GLUT_KEY_DOWN:
		break;

	default:
		break;
	}

	glutPostRedisplay();
}

//clean the GL
void CleanGL(void)
{
}

//main menu
//@param[in] id :main ID
void OnMainMenu(int id)
{
	Keyboard((unsigned char)id,0,0);
}

//create the right click menu
void InitMenu(void)
{
	//main menu
	glutCreateMenu(OnMainMenu);
	glutAddMenuEntry("------------------------",         -1);
	glutAddMenuEntry("Toggle fullscreen [f]",         'f');
	glutAddMenuEntry("Toggle animation [s]" ,           's');
	glutAddMenuEntry("Step animation [ ]",              ' ');
	glutAddMenuEntry("------------------------",         -1);
	glutAddMenuEntry("Quit [ESC]",                       '\033');
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}


int main(int argc, char *argv[])
{
	glutInitWindowPosition(g_iWinX,g_iWinY);
	glutInitWindowSize(g_iWinW,g_iWinH);
	glutInit(&argc,argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_ACCUM);
	glutCreateWindow(argv[0]);

	glutDisplayFunc(Display);
	glutReshapeFunc(Resize);
	glutMouseFunc(Mouse);
	glutMotionFunc(Motion);
	glutPassiveMotionFunc(PassiveMotion);
	glutKeyboardFunc(Keyboard);
	glutSpecialFunc(SpecialKey);

	SwitchIdle(0);

	InitGL();
	InitMenu();

	glutMainLoop();

	CleanGL();
	return 0;

}