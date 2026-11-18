// Лабораторная работа 1. FPU (x87). Вариант 2.
// Visual Studio: x86 / Win32. C++ только объявляет данные и выводит их;
// все математические вычисления выполняются в одной вставке __asm.

#include <iostream>
#include <iomanip>
#include <windows.h>
using namespace std;

int main() {
    SetConsoleOutputCP(65001);

    float x1 = 1, x2 = 3, change;
    float x[10] = {}, t[10] = {}, r[10] = {}, a[10] = {};
    float b[10] = {}, n[10] = {}, c[10] = {}, d[10] = {}, y[10] = {};
    float eps = 0.000001f;
    int tmp, valid[10] = {};

    __asm {
        finit

        // 1. Шаг: change = (x2-x1)/9
        fld dword ptr x2
        fsub dword ptr x1
        mov tmp, 9
        fild dword ptr tmp
        fdivp st(1), st(0)
        fstp dword ptr change

        // 2. Заполнение массива x[0..9]
        fld dword ptr x1
        mov ecx, 10
        mov esi, 0

    lp:
        fst dword ptr x[esi]

        // 3. Квадрат: t = (x-1)^2
        fld st(0)
        fsub dword ptr x1
        fmul st(0), st(0)
        fstp dword ptr t[esi]

        // 4. Кубический корень: r = 2^(log2(t)/3)
        fld dword ptr t[esi]
        ftst
        fstsw ax
        sahf
        jz save_root              // при t=0 корень равен нулю

        fld1
        fxch st(1)
        fyl2x                     // двоичный логарифм t
        mov tmp, 3
        fild dword ptr tmp
        fdivp st(1), st(0)        // показатель степени p = log2(t)/3

        // 2^p = 2^(p-round(p)) * 2^round(p)
        fld st(0)
        frndint
        fsub st(1), st(0)
        fxch st(1)
        f2xm1
        fld1
        faddp st(1), st(0)
        fscale
        fxch st(1)
        fstp st(0)

    save_root:
        fstp dword ptr r[esi]

        // 5. Тангенс: a = tg(r); проверка близости к разрыву тангенса
        fld dword ptr r[esi]
        fld st(0)
        fcos
        fabs
        fcomp dword ptr eps
        fstsw ax
        sahf
        jb not_domain

        fptan
        fstp st(0)
        fstp dword ptr a[esi]

        // 6. Экспонента: b = exp(3-3*x) = 2^((3-3*x)*log2(e))
        fld dword ptr x[esi]
        mov tmp, 3
        fimul dword ptr tmp
        fisubr dword ptr tmp                // 3-3*x
        fldl2e
        fmulp st(1), st(0)

        // Находим 2^p через целую и дробную части показателя p
        fld st(0)
        frndint
        fsub st(1), st(0)
        fxch st(1)
        f2xm1
        fld1
        faddp st(1), st(0)
        fscale
        fxch st(1)
        fstp st(0)
        fstp dword ptr b[esi]

        // 7. Числитель: n = a + b
        fld dword ptr a[esi]
        fadd dword ptr b[esi]
        fstp dword ptr n[esi]

        // 8. Знаменатель: c = 2*lg(10+abs(1-x^2))
        fld dword ptr x[esi]
        fmul dword ptr x[esi]
        fld1
        fsubp st(1), st(0)        // x^2-1; затем FABS берёт модуль
        fabs
        mov tmp, 10
        fiadd dword ptr tmp
        fldlg2
        fxch st(1)
        fyl2x                     // log10(2)*log2(...) = lg(...)
        fadd st(0), st(0)
        fstp dword ptr c[esi]

        // 9. Множитель: d = 3-x
        mov tmp, 3
        fild dword ptr tmp
        fsub dword ptr x[esi]
        fstp dword ptr d[esi]

        // 10. Результат: y = (n/c)*d
        fld dword ptr n[esi]
        fdiv dword ptr c[esi]
        fmul dword ptr d[esi]
        ftst                      // при нуле убираем знак -0
        fstsw ax
        sahf
        jnz save_y
        fabs
    save_y:
        fstp dword ptr y[esi]
        mov dword ptr valid[esi], 1
        jmp next_x

    not_domain:
        fstp st(0)               // удаляем r; исходный x остаётся в ST(0)
        mov dword ptr valid[esi], 0

    next_x:
        // 11. Переход к следующему x
        fadd dword ptr change
        add esi, 4
        dec ecx
        jnz lp
        fstp st(0)               // удаляем последний x из стека FPU
    }

    cout << fixed << setprecision(4);
    for (int i = 0; i < 10; i++) {
        cout << "x=" << setw(6) << x[i] << "  ";
        if (!valid[i]) {
            cout << "Значение аргумента не входит в ОДЗ\n";
            continue;
        }
        cout << "t=" << setw(7) << t[i]
             << "  r=" << setw(7) << r[i]
             << "  tg=" << setw(9) << a[i] << '\n';
        cout << "          exp=" << setw(8) << b[i]
             << "  N=" << setw(8) << n[i]
             << "  C=" << setw(8) << c[i]
             << "  D=" << setw(8) << d[i]
             << "  y=" << y[i] << '\n';
    }
    return 0;
}
