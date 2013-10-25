
struct s
{
  int a[256];
  char c[256];
};

static void
fun2 (void)
{

}

static void
fun1 (int i, int j, long k, struct s ss)
{
  /* Allocate local variables on stack.  */
  struct s s1;

  if (i < BACKTRACE_DEPTH)
    fun1 (i + 1, j + 2, k - 1, ss);
  else
    {
      int ii;

      for (ii = 0; ii < 10; ii++)
	fun2 ();
    }
}

int
main (void)
{
  struct s ss;

  fun1 (0, 0, 200, ss);
  return 0;
}
