#!/bin/sh
# Cria a pasta do repositório "synthkey-samples" (só os packs de samples + gerador).
# Uso:  sh tools/make_samples_repo.sh ../synthkey-samples
set -e
DEST="${1:?indique a pasta de destino}"
mkdir -p "$DEST/Source" "$DEST/tools" "$DEST/tests" "$DEST/Resources" "$DEST/.github/workflows"
cp -R Source/Core "$DEST/Source/"
cp tools/packgen.cpp "$DEST/tools/"
cp tests/test_packio.cpp "$DEST/tests/"
cp -R Resources/covers "$DEST/Resources/"
cp .gitignore "$DEST/.gitignore"
cp tools/samples_workflow.yml "$DEST/.github/workflows/packs.yml"
cp tools/samples_README.md "$DEST/README.md"
echo "Pronto: $DEST"
