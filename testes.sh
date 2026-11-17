#!/bin/bash
# Roda o banker em varios cenarios e mostra o resultado de cada um.
# Uso: ./testes.sh > evidencias.log 2>&1

cp customer.txt customer.bak
cp commands.txt commands.bak

restaurar() {
    cp customer.bak customer.txt
    cp commands.bak commands.txt
}

rodar() {
    echo "\$ ./banker $*"
    ./banker "$@"
    echo "(codigo de saida: $?)"
    echo
}

echo "===== COMPILACAO ====="
make clean
make
echo

echo "===== TESTE 1: execucao normal (exemplo do enunciado) ====="
rodar 10 5 7
echo "--- result.txt ---"
cat result.txt
echo

echo "===== TESTE 2: sem argumentos ====="
rodar

echo "===== TESTE 3: argumento que nao e numero ====="
rodar 10 abc 7

echo "===== TESTE 4: argumento negativo ====="
rodar 10 -5 7

echo "===== TESTE 5: menos argumentos que recursos nos arquivos ====="
rodar 10 5

echo "===== TESTE 6: mais argumentos que recursos nos arquivos ====="
rodar 10 5 7 8

echo "===== TESTE 7: customer.txt nao existe ====="
rm customer.txt
rodar 10 5 7
restaurar

echo "===== TESTE 8: commands.txt nao existe ====="
rm commands.txt
rodar 10 5 7
restaurar

echo "===== TESTE 9: customer.txt com letra no lugar de numero ====="
printf '7,5,3\n3,x,2\n' > customer.txt
rodar 10 5 7
restaurar

echo "===== TESTE 10: customer.txt separado por espaco em vez de virgula ====="
printf '7 5 3\n3 2 2\n' > customer.txt
rodar 10 5 7
restaurar

echo "===== TESTE 11: customer.txt vazio ====="
: > customer.txt
rodar 10 5 7
restaurar

echo "===== TESTE 12: customer.txt com linha de tamanho diferente ====="
printf '7,5,3\n3,2\n' > customer.txt
rodar 10 5 7
restaurar

echo "===== TESTE 13: commands.txt com comando desconhecido ====="
printf 'RQ 0 0 2 1\nXX 1 2 0 0\n' > commands.txt
rodar 10 5 7
restaurar

echo "===== TESTE 14: commands.txt com letra no lugar de numero ====="
printf 'RQ 0 0 a 1\n' > commands.txt
rodar 10 5 7
restaurar

echo "===== TESTE 15: commands.txt com quantidade errada de recursos ====="
printf 'RQ 0 0 2 1\nRQ 1 2 0\n' > commands.txt
rodar 10 5 7
restaurar

echo "===== TESTE 16: commands.txt com cliente que nao existe ====="
printf 'RQ 9 0 2 1\n' > commands.txt
rodar 10 5 7
restaurar

echo "===== TESTE 17: liberar mais recursos do que o cliente possui ====="
printf 'RQ 0 0 2 1\nRL 0 1 0 0\n*\n' > commands.txt
rodar 10 5 7
echo "--- result.txt ---"
cat result.txt
echo
restaurar

echo "===== TESTE 18: outra quantidade de clientes e recursos (2 clientes, 4 recursos) ====="
printf '6,4,7,3\n4,2,3,2\n' > customer.txt
printf 'RQ 0 2 1 3 1\nRQ 1 1 1 1 1\n*\nRL 0 1 0 1 0\nRQ 1 4 0 0 0\n' > commands.txt
rodar 8 5 9 4
echo "--- result.txt ---"
cat result.txt
echo
restaurar

rm -f customer.bak commands.bak
./banker 10 5 7
echo "===== FIM DOS TESTES ====="
