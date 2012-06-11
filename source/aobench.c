#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/timer.h>
#include <intuition/screens.h>
#include <dos/stdio.h>
#include <dos/dos.h>
#include <math.h>

// since rendering is very slow, this #define allows the display to be updated with the
// rasterized pixels as they are calculated.  this does slightly affect the overall benchmark
// speed, so if you want exact timing, then disable this #define and only the final framebuffer
// will be shown once rendering has completed.
#define DRAW_DURING_RENDERING

// if #defined, the final framebuffer output will be written to the specified file in .ppm
// format.  including this code causes the .exe to exceed 4kb, so it is not #defined by default.
//#define WRITE_PPM_OUTPUT		"DH0:ao.ppm"

#define M_PI			3.14159265358979323846
#define M_2PI			6.28318530717958647692

#define WIDTH			256
#define HEIGHT			256
#define NSUBSAMPLES		2
#define NAO_SAMPLES		8

struct DosLibrary *DOSBase = NULL;
struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase *GfxBase = NULL;
struct Device *TimerBase = NULL;


typedef struct _vec
{
	float	x;
	float	y;
	float	z;
} vec;

typedef struct _Isect
{
	float	t;
	vec		p;
	vec		n;
	int		hit;	
} Isect;

typedef struct _Sphere
{
	vec		center;
	float	radius;
} Sphere;

typedef struct _Plane
{
	vec		p;
	vec		n;
} Plane;

typedef struct _Ray
{
	vec		org;
	vec		dir;
} Ray;


static const Sphere spheres[3] = {
	// sphere 1
	{
		{ -2.0f, 0.0f, -3.5f },
		0.5f
	},
	// sphere 2
	{
		{ -0.5f, 0.0f, -3.0f },
		0.5f
	},
	// sphere 3
	{
		{ 1.0f, 0.0f, -2.2f },
		0.5f
	}
};

static const Plane plane = {
	{ 0.0f, -0.5f, 0.0f },
	{ 0.0f,  1.0f, 0.0f }
};

// drand48() function borrowed from: http://www.codeproject.com/Articles/25172/Simple-Random-Number-Generation
double drand48(void)
{
	static ULONG w = 0x1F123BB5;
	static ULONG z = 0x159A55E5;
	
	z = 36969 * (z & 0xFFFF) + (z >> 16);
	w = 18000 * (w & 0xFFFF) + (w >> 16);
	return (((z << 16) + w) + 1.0f) * 2.328306435454494e-10;
}

// dtoa() function borrowed from: http://stackoverflow.com/questions/2302969
char *dtoa(float val)
{
	const float precision = 0.001f;
	static char buf[64];
	int m = log10(val);
	char *ptr = buf;
	
	while (val > 0 + precision)
	{
		const float weight = pow(10.0f, m);
		const int digit = floor(val / weight);
		
		val -= digit * weight;
		*ptr++ = '0' + digit;
		if (m == 0)
			*ptr++ = '.';
		m--;
	}
	
	*ptr++ = '\n';
	*ptr = 0;
	return buf;
}

static void vzero(vec *c)
{
	c->x = c->y = c->z = 0.0f;
}

static void vsub(vec *c, const vec *v0, const vec *v1)
{
	c->x = v0->x - v1->x;
	c->y = v0->y - v1->y;
	c->z = v0->z - v1->z;
}

static void vmuladd(vec *c, const vec *mul, float scalar, const vec *add)
{
	c->x = add->x + mul->x * scalar;
	c->y = add->y + mul->y * scalar;
	c->z = add->z + mul->z * scalar;
}

static float vdot(const vec *v0, const vec *v1)
{
	return v0->x * v1->x + v0->y * v1->y + v0->z * v1->z;
}

static void vcross(vec *c, const vec *v0, const vec *v1)
{
	c->x = v0->y * v1->z - v0->z * v1->y;
	c->y = v0->z * v1->x - v0->x * v1->z;
	c->z = v0->x * v1->y - v0->y * v1->x;
}

static void vnormalize(vec *c)
{
	const float length = sqrt(vdot(c, c));

	if (fabs(length) > 1.0e-17) {
		c->x /= length;
		c->y /= length;
		c->z /= length;
	}
}

void
ray_sphere_intersect(Isect *isect, const Ray *ray, const Sphere *sphere, int hitonly)
{
	vec rs;

	vsub(&rs, &ray->org, &sphere->center);

	const float B = vdot(&rs, &ray->dir);
	const float C = vdot(&rs, &rs) - sphere->radius * sphere->radius;
	const float D = B * B - C;

	if (D > 0.0f) {
		const float t = -B - sqrt(D);
		
		if ((t > 0.0f) && (t < isect->t)) {
			isect->hit = 1;
			if (hitonly == 1)
				return;
			
			isect->t = t;
			
			vmuladd(&isect->p, &ray->dir, t, &ray->org);
			vsub(&isect->n, &isect->p, &sphere->center);
			vnormalize(&(isect->n));
		}
	}
}

void
ray_plane_intersect(Isect *isect, const Ray *ray, const Plane *plane, int hitonly)
{
	const float d = -vdot(&plane->p, &plane->n);
	const float v =  vdot(&ray->dir, &plane->n);

	if (fabs(v) < 1.0e-17) return;

	const float t = -(vdot(&ray->org, &plane->n) + d) / v;

	if ((t > 0.0f) && (t < isect->t)) {
		isect->hit = 1;
		if (hitonly == 1)
			return;
			
		isect->t = t;

		vmuladd(&isect->p, &ray->dir, t, &ray->org);		
		isect->n = plane->n;
	}
}

void intersect(Isect *isect, const Ray *ray, int hitonly)
{
	isect->t   = 1.0e+17;
	isect->hit = 0;

	ray_sphere_intersect(isect, ray, &spheres[0], hitonly);	
	ray_sphere_intersect(isect, ray, &spheres[1], hitonly);	
	ray_sphere_intersect(isect, ray, &spheres[2], hitonly);	
	ray_plane_intersect (isect, ray, &plane, hitonly);
}

void
orthoBasis(vec *basis, const vec *n)
{
	basis[2].x = n->x;
	basis[2].y = n->y;
	basis[2].z = n->z;
	vzero(&basis[1]);

	// no need to perform this first check as basis[1].x is set to 1.0f inside the else below
	/* if ((n->x < 0.6f) && (n->x > -0.6f)) {
		basis[1].x = 1.0f;
	} else */ if ((n->y < 0.6f) && (n->y > -0.6f)) {
		basis[1].y = 1.0f;
	} else if ((n->z < 0.6f) && (n->z > -0.6f)) {
		basis[1].z = 1.0f;
	} else {
		basis[1].x = 1.0f;
	}

	vcross(&basis[0], &basis[1], &basis[2]);
	vnormalize(&basis[0]);

	vcross(&basis[1], &basis[2], &basis[0]);
	vnormalize(&basis[1]);
}

void ambient_occlusion(vec *col, const Isect *isect)
{
	const int nthetaphi = NAO_SAMPLES * NAO_SAMPLES; 
	const float eps = 0.0001f;
	int i;
	vec p;

	vmuladd(&p, &isect->n, eps, &isect->p);		

	vec basis[3];
	orthoBasis(basis, &isect->n);

	int count = 0;

	for (i = 0; i < nthetaphi; i++) {
		const float theta = sqrt(drand48());
		const float phi   = M_2PI * drand48();

		const float x = cos(phi) * theta;
		const float y = sin(phi) * theta;
		const float z = sqrt(1.0f - theta * theta);

		// local -> global
		const float rx = x * basis[0].x + y * basis[1].x + z * basis[2].x;
		const float ry = x * basis[0].y + y * basis[1].y + z * basis[2].y;
		const float rz = x * basis[0].z + y * basis[1].z + z * basis[2].z;

		Ray ray;

		ray.org = p;
		ray.dir.x = rx;
		ray.dir.y = ry;
		ray.dir.z = rz;

		Isect occIsect;
		intersect(&occIsect, &ray, 1);
		if (occIsect.hit) count++;
	}

	const float occlusion = (nthetaphi - count) / (float)nthetaphi;
	col->x = occlusion;
	col->y = occlusion;
	col->z = occlusion;
}

unsigned char
clamp(float f)
{
	int i = (int)(f * 255.5f);
	
	if (i < 0) i = 0;
	if (i > 255) i = 255;
	
	return (unsigned char)i;
}

void
render(unsigned char *img, int w, int h, int nsubsamples, struct RastPort *rp)
{
	const float nsubsqr = (float)(nsubsamples * nsubsamples);
	int x, y;
	int u, v;
	vec fimg;

	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			vzero(&fimg);

			for (v = 0; v < nsubsamples; v++) {
				for (u = 0; u < nsubsamples; u++) {
					const float px =  (x + (u / (float)nsubsamples) - (w >> 1)) / (w >> 1);
					const float py = -(y + (v / (float)nsubsamples) - (h >> 1)) / (h >> 1);

					Ray ray;
					vzero(&ray.org);

					ray.dir.x = px;
					ray.dir.y = py;
					ray.dir.z = -1.0f;
					vnormalize(&(ray.dir));

					Isect isect;
					intersect(&isect, &ray, 0);

					if (isect.hit) {
						vec col;
						ambient_occlusion(&col, &isect);

						fimg.x += col.x;
						fimg.y += col.y;
						fimg.z += col.z;
					}
				}
			}
		
			fimg.x /= nsubsqr;
			fimg.y /= nsubsqr;
			fimg.z /= nsubsqr;
		
			// convert RGB color to luminance
			const float lumdbl = (0.299f * fimg.x) + (0.587f * fimg.y) + (0.114f * fimg.z);
			const UBYTE lum = (UBYTE)clamp(lumdbl);
			img[(y * w) + x] = lum;

#ifdef DRAW_DURING_RENDERING
			// draw the final value for this pixel to the raster port
			SetAPen(rp, lum);
			WritePixel(rp, x, y);
#endif
		}
	}
}

#ifdef WRITE_PPM_OUTPUT
void
saveppm(const char *fname, int w, int h, unsigned char *img)
{
	BPTR hdl = Open(fname, MODE_NEWFILE);
	if (hdl == 0)
		return;
	
	WriteStr("Writing output to: ");
	WriteStr(fname);
	WriteStr("...\n");
	
	const char header[] = "P5\n256 256\n255\n";
	Write(hdl, header, sizeof(header) - 1);			// -1 to not include the null terminator
	Write(hdl, img, w * h);
	Close(hdl);
}
#endif // WRITE_PPM_OUTPUT

int main()
{
	struct IORequest timereq;
	struct timeval start, end;
	struct Screen *scr;
	struct Window *wnd;
	int t;

	// the frame buffer
	static UBYTE img[WIDTH * HEIGHT];

	// open necessary libraries
	DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 36);
	IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39);
	GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 33);
	
	// open timer device
	OpenDevice("timer.device", 0, &timereq, 0);
	TimerBase = timereq.io_Device;

	// define our video mode
	struct NewScreen screen = {
		0, 0, 320, 256, 8,				// screen of 320x256 with depth	8
		DETAILPEN, BLOCKPEN,
		0,
		CUSTOMSCREEN,
		NULL,
		"",	
		NULL,
		NULL
	};

	// open the screen
	scr = OpenScreen(&screen);

	// create a window (so we can receive input messages)
	wnd = OpenWindowTags(NULL,
			WA_Left,			0,
			WA_Top,				0,
			WA_Width,			320,
			WA_Height,			256,
			WA_WindowName,		"",
			WA_Flags,			WFLG_BORDERLESS | WFLG_RMBTRAP | WFLG_BACKDROP | WFLG_ACTIVATE,
			WA_CustomScreen,	scr,
			WA_IDCMP,			IDCMP_MOUSEBUTTONS,
			TAG_DONE);

	// create and load a grayscale palette into the viewport
	struct ViewPort *vp = &scr->ViewPort;
	for (t = 0; t < 256; t++)
	{
		const ULONG val = ((ULONG)t << 24) | ((ULONG)t << 16) | ((ULONG)t << 8) | t;
		SetRGB32(vp, t, val, val, val);
	}

	// initially fill the frame buffer with grey, so we can show where rendering is currently being done
	for (t = 0; t < WIDTH * HEIGHT; t++)
		img[t] = 0x80;
		
	// blit the grey frame buffer to the raster port
	struct RastPort *rp = &scr->RastPort;
	WriteChunkyPixels(rp, 0, 0, 255, 255, img, 256);

	// start benchmark timing
	GetSysTime(&start);

	// render!
	render(img, WIDTH, HEIGHT, NSUBSAMPLES, rp);

	// end benchmark timing
	GetSysTime(&end);
	SubTime(&end, &start);

	// blit the completed frame buffer to the raster port
	WriteChunkyPixels(rp, 0, 0, 255, 255, img, 256);
	
	// wait for a mouse button to be pressed
	Wait(1 << wnd->UserPort->mp_SigBit);

	// calculate rendering time
	const float secs = (float)end.tv_secs + ((float)end.tv_micro / 1000000.0);
	
	// close our window and screen, then display the result
	CloseWindow(wnd);
	CloseScreen(scr);
	WriteStr("aobench rendering time: ");
	WriteStr(dtoa(secs));

#ifdef WRITE_PPM_OUTPUT
	// write the final image to disk
	saveppm(WRITE_PPM_OUTPUT, WIDTH, HEIGHT, img);
#endif

	// cleanup
	CloseDevice(&timereq);
	CloseLibrary((struct Library *)GfxBase);
	CloseLibrary((struct Library *)IntuitionBase);
	CloseLibrary((struct Library *)DOSBase);
	return 0;
}
