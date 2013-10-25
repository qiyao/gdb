volatile int flag = 1;

int
main (void)
{
  int i = 0;

  while (flag)
    {
      double d;
      float f;

      i++;
      d = i * 3.14;
      f = d / 0.618;
    }
  return 0;
}
