#include "headers.h"

void show_menu()
{
    printf("1. Cargar usuarios\n");
    printf("2. Cargar operaciones\n");
    printf("3. Operaciones individuales\n");
    printf("4. Generar reporte de saldos\n");
    printf("5. Salir\n");
}

void handle_user_input()
{
    int option, account1, account2;
    float amount;

    while (1)
    {
        show_menu();
        scanf("%d", &option);

        switch (option)
        {
        case 1:
            load_users_multithreaded("users.json");
            break;
        case 2:
            load_operations_multithreaded("operations.json");
            break;
        case 3:
            printf("1. Depósito\n2. Retiro\n3. Transferencia\n4. Consultar cuenta\n");
            scanf("%d", &option);
            switch (option)
            {
            case 1:
                printf("Cuenta: ");
                scanf("%d", &account1);
                printf("Monto: ");
                scanf("%f", &amount);
                if (account_exists(account1))
                {
                    deposit(account1, amount);
                }
                else
                {
                    printf("Cuenta no encontrada\n");
                }
                break;
            case 2:
                printf("Cuenta: ");
                scanf("%d", &account1);
                printf("Monto: ");
                scanf("%f", &amount);
                if (account_exists(account1))
                {
                    withdraw(account1, amount, 0);
                }
                else
                {
                    printf("Cuenta no encontrada\n");
                }
                break;
            case 3:
                printf("Cuenta origen: ");
                scanf("%d", &account1);
                printf("Cuenta destino: ");
                scanf("%d", &account2);
                printf("Monto: ");
                scanf("%f", &amount);
                if (account_exists(account1) && account_exists(account2))
                {
                    transfer(account1, account2, amount, 0);
                }
                else
                {
                    printf("Cuenta no encontrada\n");
                }
                break;
            case 4:
                printf("Cuenta: ");
                scanf("%d", &account1);
                User *user = get_user(account1);
                if (user)
                {
                    printf("Cuenta: %d, Nombre: %s, Saldo: %.2f\n", user->account_number, user->name, user->balance);
                }
                else
                {
                    printf("Cuenta no encontrada\n");
                }
                break;
            }
            break;
        case 4:
            generate_balance_report();
            break;
        case 5:
            return;
        default:
            printf("Opción no válida\n");
        }
    }
}

int main()
{
    pthread_mutex_init(&lock, NULL);

    handle_user_input();

    pthread_mutex_destroy(&lock);
    return 0;
}
