# Histórico do relatório

Relatório técnico "Inserção adaptativa de elementos de interface em malhas 2D". A versão atual fica em
`doc/report/relatorio.pdf` (fontes em `doc/report/`), e o PDF de cada versão publicada fica nesta pasta como
`relatorio_vX.Y.pdf`.

Para publicar uma nova versão:
1. incrementar a macro `\versao` em `doc/report/relatorio.tex`;
2. acrescentar uma linha na tabela de histórico logo após o resumo (`relatorio.tex`) e uma entrada abaixo;
3. regenerar as figuras (`tools/gen.py`, ver Apêndice A do relatório) e compilar;
4. copiar o PDF compilado para `historico/relatorio_vX.Y.pdf`, sem apagar as versões anteriores.

## Pendências para a próxima versão (v0.3)

Itens pedidos pelo autor para entrar quando o relatório for gerado novamente. **Não gerar o relatório antes de o autor
pedir.**

- [ ] Adicionar imagens que exemplifiquem as hipóteses $x_1$ e $x_3$ do Split Element (`retrieveAvailableVertex`).
  A Figura 9 da v0.2 já mostra as duas hipóteses numa malha (antes/depois); a ideia é ter figuras dedicadas, em
  detalhe, mostrando `he`, `b`, `prev(he)`/`next(b)`, `x1`/`x3`, a aresta colapsada `c` e o elemento compartilhado `S`.
- [ ] Refletir no relatório as modificações de código feitas desde a v0.2.

## v0.2 — 18/10/2026

- A análise passou a ser feita sobre `IBHMInterfaceElementBuilder`, a implementação que será evoluída. A implementação
  baseada na CHE (`CHE`, `CHEInterfaceElementBuilder`) foi um experimento inicial e saiu do relatório; a teoria da CHE
  foi mantida.
- Nova seção 2.5 com os vetores da IBHM (V, O, Of) antes e depois de uma inserção.
- Algoritmo de travessia descrito com a validação de entrada e o `commitElement(4)` antes dos operadores.
- Verificação refeita só com a IBHM: 800 malhas aleatórias (quadriláteros, triângulos, híbridas e com hexágonos),
  três ordens mais uma inserção em lote por malha; experimento Split → Expand com 791 ocorrências.
- Novos casos-limite: índices de half-edge inválidos ou no espaço reservado, malha com furo interno.
- Comentários do código revisados em `IBHMInterfaceElementBuilder.h` e `IBHM.h` (B1–B14) e defeitos restritos à IBHM
  (I1–I10).
- Ferramentas (`tools/driver.cpp`, `gen.py`, `stress.py`) passaram a usar só a IBHM; o driver ganhou `--batch`.

## v0.1 — 18/10/2026

- Primeira versão, baseada em `CHEInterfaceElementBuilder`: introdução ao problema, IBHM, arquitetura, operadores,
  travessia, verificação (600 casos) e confronto documentação × implementação.
