#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "jni.h"

extern "C" void __cxa_pure_virtual(void) { abort(); }

void operator delete(void*) { abort(); }

#ifdef JNI_VERSION_1_6
// todo: use JavaVMInitArgs instead
typedef struct JDK1_1InitArgs {
    jint version;

    char **properties;
    jint checkSource;
    jint nativeStackSize;
    jint javaStackSize;
    jint minHeapSize;
    jint maxHeapSize;
    jint verifyMode;
    char *classpath;

    jint (JNICALL *vfprintf)(FILE *fp, const char *format, va_list args);
    void (JNICALL *exit)(jint code);
    void (JNICALL *abort)(void);

    jint enableClassGC;
    jint enableVerboseGC;
    jint disableAsyncGC;
    jint verbose;
    jboolean debugging;
    jint debugPort;
} JDK1_1InitArgs;
#endif

namespace {

void
usageAndExit(const char* name)
{
  fprintf(stderr, "usage: %s [-cp <classpath>] [-Xmx<maximum heap size>] "
          "<class name> [<argument> ...]\n", name);
  exit(-1);
}

} // namespace

int
main(int ac, const char** av)
{
  JDK1_1InitArgs vmArgs;
  vmArgs.version = 0x00010001;
  JNI_GetDefaultJavaVMInitArgs(&vmArgs);

  const char* class_ = 0;
  int argc = 0;
  const char** argv = 0;
  int propertyCount = 0;

  for (int i = 1; i < ac; ++i) {
    if (strcmp(av[i], "-cp") == 0) {
      vmArgs.classpath = const_cast<char*>(av[++i]);
    } else if (strncmp(av[i], "-Xmx", 4) == 0) {
      vmArgs.maxHeapSize = atoi(av[i] + 4);
    } else if (strncmp(av[i], "-D", 2) == 0) {
      ++ propertyCount;
    } else {
      class_ = av[i++];
      if (i < ac) {
        argc = ac - i;
        argv = av + i;
        i = ac;
      }
    }
  }

  const char* properties[propertyCount + 1];
  properties[propertyCount] = 0;
  for (int i = 1; i < ac; ++i) {
    if (strncmp(av[i], "-D", 2) == 0) {
      properties[--propertyCount] = av[i] + 2;
    }
  }

  vmArgs.properties = const_cast<char**>(properties);

  if (class_ == 0) {
    usageAndExit(av[0]);
  }

  JavaVM* vm;
  void* env;
  JNI_CreateJavaVM(&vm, &env, &vmArgs);
  JNIEnv* e = static_cast<JNIEnv*>(env);

  jclass c = e->FindClass(class_);
  if (not e->ExceptionOccurred()) {
    jmethodID m = e->GetStaticMethodID(c, "main", "([Ljava/lang/String;)V");
    if (not e->ExceptionOccurred()) {
      jclass stringClass = e->FindClass("java/lang/String");
      if (not e->ExceptionOccurred()) {
        jobjectArray a = e->NewObjectArray(argc, stringClass, 0);
        if (not e->ExceptionOccurred()) {
          for (int i = 0; i < argc; ++i) {
            e->SetObjectArrayElement(a, i, e->NewStringUTF(argv[i]));
          }
          
          e->CallStaticVoidMethod(c, m, a);
        }
      }
    }
  }

  int exitCode = 0;
  if (e->ExceptionOccurred()) {
    exitCode = -1;
    e->ExceptionDescribe();
  }

  vm->DestroyJavaVM();

  return exitCode;
}
