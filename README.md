<h1 align="center"> Criando ambientes virtuais de conversação com uso system call select() </h1>

## Inicializar o servidor

#### 1 - Compile o código:
$ gcc -o server server.c

#### 2 - Inicie o server:
$ ./server [ip do server] [porta do server]

## Conectando o servidor com o cliente

Ao entrar na concexão você terá que inserir seu nome e inserir o número da sala desejada, se não criar uma sala, colocando o limite e o ID.

### Conecte-se ao server
$ telnet [ip do server] [porta do server]

### Insira seu nome
$ Ana

### Insira a sala desejada (-1 caso queira criar uma nova)
$ -1

### Insira o limite da sala
$ 10

Após entrar em uma sala poderá conversar a vontade:

### Converse à vontade com os integrantes da mesma sala
$ Ola pessoal

## Comando do server:

O server possui implementado alguns comandos, podendo listar os usuários que estão na sala, trocar de sala e sair delas:

### Listagem dos integrantes
$ /listar

### Ex. de resposta
$ ===== Clientes Conectados Na Sala =====
$ Ana
$ Pedro
$ Lucas

### Trocar de sala (logo depois forneça o número da sala)
$ /trocar_sala
$ 4

### Sair de uma sala
$ /sair
