# DREVKANO: A ÚLTIMA QUEBRA

Jogo original para Sega Mega Drive / Genesis, escrito em C com SGDK. A imagem enviada serviu apenas de referência de atmosfera e composição; código, nomes, personagens, cenários e sinais visuais deste projeto foram criados sem copiar recursos de outras franquias.

## Estado real desta entrega

O primeiro marco é uma fase curta em desenvolvimento com título, movimentação em profundidade, inimigos que perseguem e atacam, soco, chute, defesa, golpe especial, salto, vida, pontuação e estados de vitória/derrota/reinício. A arte é uma composição provisória de blocos/tiles desenhados por código e os sons são tons PSG simples. **Não é ainda o jogo completo pedido:** faltam sprites finais de pixel art e todas as animações refinadas (corrida, golpes, queda), cenários com obstáculos sólidos e scroll, música original composta, mais fases e testes em emulador/hardware.

A ROM só deve ser considerada gerada quando o workflow `Build DREVKANO ROM` terminar com sucesso e salvar o artefato `DREVKANO-Mega-Drive-ROM`. O sucesso da compilação não equivale a teste de gameplay em emulador ou console.

## Controles planejados/implementados

- Direcional: deslocamento lateral; baixo ajusta profundidade.
- A: soco; B: chute.
- C: defesa; C junto de A+B: especial (consome vida).
- Cima: salto.
- Start: iniciar/reiniciar.

## Estrutura

- `src/`: jogo C/SGDK.
- `assets/sprites`, `assets/backgrounds`, `assets/audio`: pastas para material original.
- `docs/`: escopo e checklist.
- `.github/workflows/`: compilação no toolchain SGDK-compatível.
- `build/`: destino local da ROM; ficará vazio até uma compilação validada.

## Build local

Com SGDK instalado, defina `GDK` para a pasta SDK e rode `make rom`. A saída deve ser `build/DREVKANO_A_ULTIMA_QUEBRA.bin`. O projeto usa o makefile padrão da SGDK.
