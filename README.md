<h1 align="center"> Criando ambientes virtuais de conversação com uso system call select() </h1>

## Inicializar o servidor

### Compile o código:

```
$ gcc -o server server.c
```

### Inicie o server:

```
$ ./server [ip do server] [porta do server]
```

## Conectando o servidor com o cliente (telnet)

### Conecte-se ao server como cliente

```
$ telnet [ip do server] [porta do server]

Insira seu nome
$ Ana

Insira o nome da sala ou /criar para criar uma nova sala
$ /criar

Se estiver criando de um nome para sala
$ sala01

Insira o limite da sala
$ 10
```

### Converse à vontade com os integrantes da mesma sala

```
$ Ola pessoal
[Ana] => Olá a todos!!
[Ricardo] => Saudações =)
```

## Comando do server:

O server possui implementado alguns comandos, podendo listar os usuários que estão na sala, trocar de sala e sair delas:

```
Listagem dos integrantes
$ /listar

Ex. de resposta
$ ===== Clientes Conectados Na Sala =====
$ Ana
$ Ricardo
$ Lucas

Trocar de sala (logo depois forneça o nome da sala)
$ /trocar_sala
$ sala02

Sair de uma sala
$ /sair
```
