int main(int argc, char **argv)
{
    int a = 1;
    int b = 2;
    int c;

    switch (argc) {
        case 2:
        case 3:
        case 4:
            c = argc;
        break;
        default:
            c = 0;
        break;
    }

    return a + b + c;
}