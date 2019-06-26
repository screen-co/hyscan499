#include <Windows.h>
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <stdio.h>
#include <strsafe.h>

#define IDI_ICON  L"IDI_ICON1"
#define BITMAP_ID L"BITMAP_ID"
#define CONFIG_FILENAME L"launcher.ini"

#define L_ERROR(msg) MessageBoxW (NULL, (msg), L"Error", MB_ICONERROR)

typedef struct
{
  wchar_t **data;
  size_t    len;
} LauncherArray;

/* Конфигурация запуска. */
struct
{
  LauncherArray dlls;
  LauncherArray paths;
  LauncherArray cmds;
} config;


struct {
  BOOL      loaded;    /* Признак того, что получилось собрать GUI. */
  HINSTANCE instance;
  HWND      splash;
  HWND      p_bar;

  HBITMAP   bmp;
  LONG      bmp_width;
  LONG      bmp_height;
  HDC       splash_dc;
  HDC       mem_dc;
} app;

/* Обработка сообщений к сплэш-скрину. */
LRESULT CALLBACK
launcher_wnd_proc (HWND   wnd,
                   UINT   msg,
                   WPARAM w_param,
                   LPARAM l_param)
{

  switch (msg)
    {
    case WM_ERASEBKGND:
      /* Фон в виде изображения. */
      BitBlt ((HDC) w_param, 0, 0, app.bmp_width, app.bmp_height, app.mem_dc, 0, 0, SRCCOPY);
      break;

    default:
      return (DefWindowProc (wnd, msg, w_param, l_param));
    }

  return 0;

}

/* Добавляет путь в PATH. */
static void
launcher_path_prepend (LPWSTR prepend_path)
{
  WCHAR buffer[10];

  size_t new_path_len;
  size_t old_path_len;
  LPWSTR old_path;
  LPWSTR new_path;

  LPCWSTR env_name = L"PATH";

  old_path_len = GetEnvironmentVariableW (env_name, buffer, sizeof (buffer) / sizeof (*buffer));
  old_path = (LPWSTR) malloc (old_path_len * sizeof (WCHAR));
  if (!GetEnvironmentVariableW (env_name, old_path, old_path_len))
     {
       L_ERROR (L"Failed to get PATH variable");

       return;
     }

  /* Создаём пустую строку для новой переменной нужного размера. */
  new_path_len = wcslen (prepend_path) + 1 + wcslen (old_path) + 1;
  new_path = (LPWSTR) malloc (new_path_len * sizeof (WCHAR));
  *new_path = L'\0';

  /* Склеиваем новый путь. */
  StringCchCatW (new_path, new_path_len, prepend_path);
  StringCchCatW (new_path, new_path_len, L";");
  StringCchCatW (new_path, new_path_len, old_path);
  
  if (!SetEnvironmentVariableW (env_name, new_path))
    {
      printf ("SetEnvironmentVariable failed (%lu)\n", GetLastError());
      return;
  }

  free (old_path);
  free (new_path);
}

/* Запускает процесс. */
static BOOL
launcher_process_create (LPWSTR command,
                         BOOL   wait_)
{
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  BOOL fSuccess;

  // Create a child process.
  SecureZeroMemory (&si, sizeof(STARTUPINFOW));
  si.cb = sizeof (STARTUPINFOW);

  fSuccess = CreateProcessW (NULL, command, NULL, NULL, TRUE, CREATE_UNICODE_ENVIRONMENT,
                             NULL, NULL, &si, &pi);
  if (!fSuccess)
    return FALSE;

  if (wait_)
    WaitForSingleObject (pi.hProcess, INFINITE);

  return fSuccess;
}

/* Строит GUI сплэш-скрина. */
static void
launcher_app_init (HINSTANCE hInstance)
{
  WNDCLASSEXW splash_class;
  RECT desktop_rect;
  BITMAP bitmap;
  
  app.instance = hInstance;

  /* Регистрируем класс SplashWindowClass. */
  splash_class.cbSize = sizeof (WNDCLASSEXW);
  splash_class.style = 0;
  splash_class.lpfnWndProc = (WNDPROC) launcher_wnd_proc;
  splash_class.cbClsExtra = 0;
  splash_class.cbWndExtra = 0;
  splash_class.hInstance = app.instance;
  splash_class.hIcon = LoadIconW (app.instance, IDI_ICON);
  splash_class.hIconSm = LoadIconW (app.instance, IDI_ICON);
  splash_class.hCursor = LoadCursor (NULL, IDC_WAIT);
  splash_class.hbrBackground = NULL;
  splash_class.lpszMenuName = NULL;
  splash_class.lpszClassName = L"SplashWindowClass";

  if (!RegisterClassExW (&splash_class))
    {
      L_ERROR (L"Failed To Register The Splash Window Class.");
      return;
    }

  /* Получаем информацию по изображению сплэш-скрина. */  
  app.bmp = LoadBitmapW (app.instance, BITMAP_ID);
  if (!app.bmp)
    {
      L_ERROR (L"Failed To Load Bitmap");
      return;
    }

  GetObjectW (app.bmp, sizeof (BITMAP), &bitmap);
  app.bmp_width = bitmap.bmWidth;
  app.bmp_height = bitmap.bmHeight;

  GetWindowRect (GetDesktopWindow (), &desktop_rect);
  app.splash = CreateWindowExW (0, splash_class.lpszClassName, L"Splash Screen", WS_POPUP,
                                (desktop_rect.right - app.bmp_width) / 2,
                                (desktop_rect.bottom - app.bmp_height) / 2,
                                bitmap.bmWidth, bitmap.bmHeight + 20,
                                NULL, NULL, app.instance, NULL);

  if (!app.splash)
    {
      L_ERROR (L"Splash Window Creation Failed.");
      return;
    }

  app.p_bar = CreateWindowExW (0, PROGRESS_CLASSW, NULL, WS_CHILD | WS_VISIBLE,
                               0, app.bmp_height - 20, app.bmp_width, 20,
                               app.splash, NULL, app.instance, NULL);


  app.splash_dc = GetDC (app.splash);
  app.mem_dc = CreateCompatibleDC (app.splash_dc);
  SelectObject (app.mem_dc, (HGDIOBJ) app.bmp);

  ShowWindow (app.splash, SW_SHOW);
  UpdateWindow (app.splash);

  app.loaded = TRUE;
}

/* Определяет путь к директории с программой. */
static LPWSTR
launcher_get_module_dir (void)
{
  LPWSTR filename;
  DWORD filename_len;

  filename = (LPWSTR) malloc (MAX_PATH  * sizeof (WCHAR));
  filename_len = GetModuleFileNameW (NULL, filename, MAX_PATH);
  if (MAX_PATH < filename_len)
    {
      filename = (LPWSTR) realloc (filename, filename_len * sizeof (WCHAR));

      filename_len = GetModuleFileNameW (NULL, filename, filename_len);
      if (!filename_len)
        {
          free (filename);
          return NULL;
        }
    }

  PathRemoveFileSpecW (filename);

  return filename;
}

/* Добавляет элемент в массив. */
static void
launcher_array_push (LauncherArray *array,
                     const wchar_t *value)
{
  if (array->data == NULL)
    array->data = malloc (sizeof (*array->data));
  else
    array->data = realloc (array->data, (array->len + 1) * sizeof (*array->data));

  array->data[array->len++] = _wcsdup (value);
}

/* Удаляет в конце строки ненужные символы. */
static void
launcher_trim (wchar_t *line)
{
  wchar_t *end;

  end = line + wcslen (line) - 1;
  while ((*end == L'\n') || (*end == L'\r') || (*end == L' '))
    {
      *end = L'\0';

      if (end == line)
        break;

      --end;
    }
}

static void
launcher_read_config (const wchar_t *file_name)
{
  FILE *handle;
  errno_t err;
  wchar_t line[255];

  enum { MODE_INVALID = - 1, MODE_DLL, MODE_CMD, MODE_PATH, MODE_N } mode = MODE_INVALID;

  err = _wfopen_s (&handle, file_name, L"r");
  if (err != 0)
    {
      L_ERROR (L"Failed to open launch configuration");
      return;
    }

  while (fgetws (line, 255, handle) != NULL)
    {
      launcher_trim (line);
      if (line[0] == L'\0')
        continue;

      /* Определяем группу. */
      if (wcscmp (L"[dll]", line) == 0)
        mode = MODE_DLL;
      else if (wcscmp (L"[cmd]", line) == 0)
        mode = MODE_CMD;
      else if (wcscmp (L"[path]", line) == 0)
        mode = MODE_PATH;

      /* Считываем строки нужной группы. */
      else if (mode == MODE_DLL)
        launcher_array_push (&config.dlls, line);
      else if (mode == MODE_CMD)
        launcher_array_push (&config.cmds, line);
      else if (mode == MODE_PATH)
        launcher_array_push (&config.paths, line);
    }


  fclose (handle);
}

INT WINAPI
WinMain (HINSTANCE hInstance,
         HINSTANCE hPrevInstance,
         LPSTR     lpCmdLine,
         INT       nCmdShow)
{
  LPWSTR module_dir;

  launcher_app_init (hInstance);

  module_dir = launcher_get_module_dir ();

  /* Читаем конфиг. */
  {
    WCHAR config_path[MAX_PATH];

    wcscpy_s (config_path, MAX_PATH, module_dir);
    PathAppendW (config_path, CONFIG_FILENAME);

    launcher_read_config (config_path);
  }

  /* Меняем PATH. */
  if (config.paths.len > 0)
    {
      int i;

      for (i = 0; i < config.paths.len; ++i)
        {
          WCHAR path_append[MAX_PATH];

          wcscpy_s (path_append, MAX_PATH, module_dir);
          PathAppendW (path_append, config.paths.data[i]);
          launcher_path_prepend (module_dir);
        }
    }

  /* Загружаем DLL. */
  if (config.dlls.len > 0)
    {
      int i;

      if (app.loaded)
        {
          SendMessageW (app.p_bar, PBM_SETRANGE, 0, MAKELPARAM (0, config.dlls.len));
          SendMessageW (app.p_bar, PBM_SETSTEP, (WPARAM) 1, 0);
        }

      for (i = 0; i < config.dlls.len; ++i)
        {
          LoadLibraryW (config.dlls.data[i]);

          if (app.loaded)
            SendMessageW (app.p_bar, PBM_STEPIT, 0, 0);
        }
    }

  /* Закрываем сплэш-скрин. */
  if (app.loaded)
    SendMessageW (app.splash, WM_CLOSE, 0, 0);

  /* Выполняем команды. */
  if (config.cmds.len > 0)
    {
      int i;

      for (i = 0; i < config.cmds.len; ++i)
        launcher_process_create(config.cmds.data[i], i < config.cmds.len - 1);
    }


  return 0;
}
