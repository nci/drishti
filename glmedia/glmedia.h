
#ifndef glmedia_h
#define glmedia_h

#ifdef WIN32
#ifdef GLMEDIA_EXPORTS
#define GLMEDIA_API __declspec(dllexport)
#else
#define GLMEDIA_API __declspec(dllimport)
#endif // GLMEDIA_EXPORTS
#else
#define GLMEDIA_API
#endif // WIN32

#if !defined(WIN32) && defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*
 * Debug messages
 */

GLMEDIA_API
void glmedia_debug_redirect(void (*debug_function)(const char *, void *),
			    void *user_data);


/*
 * Output images
 */
struct _glmedia_image_writer;
typedef struct _glmedia_image_writer* glmedia_image_writer_t;

GLMEDIA_API 
glmedia_image_writer_t 
glmedia_image_writer_create();

GLMEDIA_API 
int
glmedia_image_writer_write(glmedia_image_writer_t writer,
			   const char *filename,
			   unsigned int width,
			   unsigned int height,
			   unsigned int quality,
			   void *data);

GLMEDIA_API 
void
glmedia_image_writer_free(glmedia_image_writer_t writer);

/*
 * Output movies
 */
struct _glmedia_movie_writer;
typedef struct _glmedia_movie_writer* glmedia_movie_writer_t;

GLMEDIA_API 
glmedia_movie_writer_t
glmedia_movie_writer_create();

GLMEDIA_API 
int
glmedia_movie_writer_start(glmedia_movie_writer_t writer,
			   const char *fname,
			   unsigned int width,
			   unsigned int height,
			   unsigned int fps,
			   unsigned int quality);

GLMEDIA_API 
int 
glmedia_movie_writer_add(glmedia_movie_writer_t writer,
			 void *data);

GLMEDIA_API 
int 
glmedia_movie_writer_end(glmedia_movie_writer_t writer);

GLMEDIA_API 
void
glmedia_movie_writer_free(glmedia_movie_writer_t writer);

/*
 * Framebuffer for rendering scenes at arbitrary sizes.
 */

struct _glmedia_fbo;
typedef struct _glmedia_fbo* glmedia_fbo_t;

GLMEDIA_API extern const int GLMEDIA_DEFAULT_TARGET;

GLMEDIA_API 
glmedia_fbo_t 
glmedia_fbo_create(int target, 
		   unsigned int cell_size);

GLMEDIA_API 
int 
glmedia_fbo_resize(glmedia_fbo_t fbo,
		   unsigned int cell_size);

GLMEDIA_API 
int 
glmedia_fbo_draw(glmedia_fbo_t fbo,
		 unsigned int width,
		 unsigned int height,
		 float *frustum,
		 float *model_view,
		 void (*render)(void*),
		 void *render_arg,
		 void *img);

GLMEDIA_API 
void 
glmedia_fbo_free(glmedia_fbo_t fbo);

#if !defined(WIN32) && defined(__cplusplus)
};
#endif /* __cplusplus */
			   
#endif // glmedia_h
