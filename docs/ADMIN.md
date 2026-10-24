# Administrador — chaves de packs

## Ver o separador ADMIN
1. Abra `PRIVADO_senhas_SYNTHKEY.txt` (guardado só por si) e copie a linha do *token de administrador*.
2. Crie um ficheiro de texto `admin.key` com **apenas** essa linha, em:
   * macOS: `~/Library/Application Support/Piradex/SYNTHKEY/admin.key`
   * Windows: `%APPDATA%\Piradex\SYNTHKEY\admin.key`
3. Abra o plugin, entre com a senha **MASTER** → aparece o separador **ADMIN**.

## Gerar chaves
Escolha os packs (um ou vários), a validade (permanente · até uma data · N dias após a ativação),
o nº de computadores, a quantidade e uma nota (nome do cliente). **GERAR CHAVES** mostra a lista;
**COPIAR** / **EXPORTAR CSV**. O registo fica em `issued.json` na mesma pasta.

**Simular cliente** ignora o acesso MASTER/BETA aos packs para testar uma chave como um cliente.

## Dar um pack a um cliente
Envie o ficheiro `SYNTHKEY_Pack_<estilo>_1.1.0.bfpack` + a chave. O cliente arrasta o `.bfpack` para a janela
do plugin (ou usa PACKS → INSTALAR), depois clica no cartão do estilo → **Inserir chave**.
