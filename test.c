int v;

int multiline(int a) {
    a --;
    a --;
    a ++;
    return a;
}

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
        a = sum(i,b);
        b = multiline(a);
    }
    for (int j = 0; j < 3; j ++) {
        int c = a + b;
        b = a - j + c;
    }

    return 0;
}