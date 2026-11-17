# Algoritmo do Banqueiro

Implementação 5 da disciplina de Infraestrutura de Software (CESAR School).

Implementação em C do algoritmo do banqueiro (Operating System Concepts, Silberschatz et al., 10a edição), usado para evitar deadlock: um pedido de recursos só é atendido se o sistema continuar em estado seguro.

## Compilação

```
make
```

Gera o executável `banker`. Para apagar o executável e o `result.txt`:

```
make clean
```

## Execução

Cada argumento é o número de instâncias de um tipo de recurso. A quantidade de argumentos define a quantidade de recursos.

```
./banker 10 5 7
```

O programa lê dois arquivos da pasta atual e escreve a saída em `result.txt`.

### customer.txt

Uma linha por cliente, com a demanda máxima de cada recurso separada por vírgula. O número de linhas define o número de clientes.

```
7,5,3
3,2,2
9,0,2
```

### commands.txt

Um comando por linha:

- `RQ <cliente> <recursos...>`: o cliente pede recursos
- `RL <cliente> <recursos...>`: o cliente libera recursos
- `*`: escreve o estado atual (tabelas MAXIMUM, ALLOCATION, NEED e AVAILABLE)

```
RQ 0 0 2 1
RL 0 0 1 1
*
```

## Tratamento de erros

O programa valida os argumentos e os dois arquivos antes de executar qualquer comando. Em caso de erro, mostra uma mensagem no terminal e encerra com código 1.

| Situação | Mensagem |
|---|---|
| Nenhum argumento | `Error: inform the number of instances of each resource...` |
| Argumento que não é inteiro não negativo | `Error: invalid argument "..."...` |
| `customer.txt` inexistente, vazio ou fora do formato | `Fail to read customer.txt` |
| `commands.txt` inexistente ou fora do formato | `Fail to read commands.txt` |
| Colunas do `customer.txt` diferentes da quantidade de argumentos | `Incompatibility between customer.txt and command line` |
| Recursos de um comando diferentes da quantidade de argumentos | `Incompatibility between commands.txt and command line` |
| Comando para um cliente que não existe | `Error: customer N in commands.txt does not exist` |

## Arquivos

- `banker.c`: código-fonte
- `Makefile`: compilação (`make`) e limpeza (`make clean`)
- `testes.sh`: script que roda os cenários de teste
- `evidencias.log`: saída do `testes.sh`, gerada com `./testes.sh > evidencias.log 2>&1`
