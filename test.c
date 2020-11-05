int v;

int sum(int x, int y) {
    return x + y;
}

int main()
{
    int a = v;
    {
        int nested = 5;
    }
    int b = 1;
    for (int i = 0; i < 5; i ++) {
        a = i;
        b = i+1;
        int c = a + b;
    }
}