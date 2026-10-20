# Histórico do relatório

Relatório técnico "Inserção adaptativa de elementos de interface em malhas 2D". A versão atual fica em
`doc/report/relatorio.pdf` (fontes em `doc/report/`), e o PDF de cada versão publicada fica nesta pasta como
`relatorio_vX.Y.pdf`.

Para publicar uma nova versão:
1. incrementar a macro `\versao` em `doc/report/relatorio.tex`;
2. acrescentar uma linha na tabela de histórico logo após o resumo (`relatorio.tex`) e uma entrada abaixo;
3. regenerar as figuras (`tools/gen.py`, ver Apêndice A do relatório) e compilar;
4. copiar o PDF compilado para `historico/relatorio_vX.Y.pdf`, sem apagar as versões anteriores.

## Sugestões registradas para implementação futura

Sugestões discutidas com o autor e deixadas para depois (decisão dele, 20/10/2026). Não implementar sem o autor pedir.

### I7: relato do resultado de `insertInterfaceElements`

- **Problema.** O método devolve `void`. Quem chama não sabe quais arestas viraram elementos, quais foram rejeitadas e
  por quê; as rejeições só aparecem no console (`printf`).
- **Por que não basta devolver a contagem ou a lista dos elementos inseridos.** Os elementos novos sempre entram no fim
  e em sequência: são os índices de `numberOfElements()` antes da chamada até `numberOfElements()` depois. Quem chama já
  obtém a contagem e a lista comparando esses dois valores. O que falta é a correspondência aresta → elemento (as
  arestas rejeitadas são puladas, então o elemento k não corresponde a `edges[k]`) e o motivo de cada rejeição. Na
  integração com o GeMA, isso será necessário para associar cada elemento de interface à fratura, falha ou lei coesiva
  da aresta de origem.
- **Sugestão.** Uma entrada por aresta de entrada, na mesma ordem:
  - o índice do elemento de interface criado para ela; ou
  - um código de rejeição: índice que não é half-edge da malha (`h >= numberOfHalfEdges()` no início da chamada);
    aresta de borda (oposta `BORDER`); aresta de elemento de interface (já aberta, inclusive repetida no mesmo lote).
- **Forma.** Um `std::vector<unsigned int>` do tamanho da entrada, com sentinelas no estilo de `IBHM::BORDER` e
  `IBHM::COLLAPSED` (por exemplo, valores no fim da faixa de `unsigned`), ou um pequeno `struct` {elemento, estado}.
  Para não alocar quando ninguém usa, pode ser um parâmetro de saída opcional (ponteiro, `nullptr` por padrão). Custo:
  uma escrita por aresta, fora dos operadores (não toca o caminho crítico).
- **Relação com o B10.** Hoje os operadores devolvem `true` e o chamador faz `assert` (decisão provisória). Se um dia os
  operadores relatarem falhas, o mesmo vetor pode ganhar um código "operador não aplicável" no lugar da asserção.
- **Mesmo padrão para o roteiro (item 2).** O mapa "vértice novo → vértice de origem", para transferir campos nodais
  (deslocamentos, poropressões etc.), pode ser devolvido do mesmo jeito: uma entrada por vértice novo com o vértice
  original. O construtor conhece essa informação no momento da criação (`duplicatePoint(v)`).

### Rejeições impressas com `printf`

- **Não há problema de correção nem de desempenho:** o `printf` só roda quando uma aresta é rejeitada.
- **Problemas de uso como biblioteca:**
  1. Quem chama não fica sabendo: a informação não volta para o programa (é o I7).
  2. O canal não é controlado por quem chama: vai para a saída padrão do programa hospedeiro (GeMA). Polui saídas e
     logs, se perde numa aplicação gráfica no Windows (sem console) e se embaralha em execuções em paralelo.
     Diagnóstico costuma ir para a saída de erro ou para o log do hospedeiro.
  3. Volume: numa análise adaptativa que repete pedidos de arestas já abertas, podem sair milhares de linhas.
  4. Testes: as mensagens aparecem no meio da saída do Google Test e não podem ser verificadas.
- **Sugestão.** Enquanto o projeto está em desenvolvimento, manter o `printf` (ajuda a depurar). Ao fazer o I7, a
  biblioteca deixa de imprimir e devolve os códigos de rejeição; o registro fica com quem chama (GeMA, `driver`,
  testes). Passo intermediário possível, se incomodar antes: imprimir na saída de erro (`fprintf(stderr, ...)`).

### Outros itens deixados para depois

- **I9 / C3 (desempenho):** cada `next`/`prev` refaz a busca binária em `Of[]`; aproveitar o elemento corrente nas
  rotações e manter o vetor `VH[]` atualizado pelos operadores (elimina as rotações de `isBorder`).
- **I12, pico de memória na construção da IBHM:** `buildOppositesLinear` monta uma tabela temporária com um `std::vector` por
  vértice (cerca de 36 MB de pico extra numa malha de 1M elementos, medido em 20/10/2026). Trocar por dois vetores
  planos (contagem de half-edges por vértice e índices).
- **Menores:** D1 (vértice 18 de `malha2Hyb.txt` em (3,4), deveria ser (4,3)); D3 (`readHybridMesh` imprime os offsets
  a cada elemento); `@todo` do construtor (nome do `commitElement`, garantir um único construtor por malha, eliminar o
  mapa `_collapsed`).

## Pendências para a próxima versão (v0.6)

Itens pedidos pelo autor para entrar quando o relatório for gerado novamente. **Não gerar o relatório antes de o autor
pedir.**

- (nenhum por enquanto)

### Modificações de código desde a v0.5

- (nenhuma por enquanto)

## v0.5 — 20/10/2026

- Reflete as correções I1 (Open Hole em vértice de borda), I2 (`isBorder`), I6 (sentinelas com asserções), I11
  (crescimento exato, `reserveCapacityForElements`) e o B10 tratado com asserções (provisório), nas seções 2, 3, 4, 5 e
  7: tabela de diferenças da IBHM, máquina de estados sem o estado "não suportado" (com os lados curtos por estado),
  Open Hole com as duas paredes (elemento de interface ou borda) e o Algoritmo 5 novo, algoritmo de inserção com as
  asserções, parágrafo de memória com a medição da malha de 1M elementos.
- Seção 6: tabela do estresse com as colunas v0.4/v0.5 (rodada 1 agora 400/400, 148 casos de borda); cenários
  atualizados; seção 6.3 nova: suítes (91 testes), o oráculo da topologia esperada, a tabela princípio → teste, o teste
  de mutação e o controle negativo ampliado (I1, I2, I6); seção 6.4 "vértice de borda com duas arestas" com figura
  antes/depois.
- Seção 7: B8 e B10 atualizados; defeitos em aberto I7 (com a sugestão combinada e a análise do `printf`), I9 e o novo
  I12 (pico de memória da construção); I1, I2, I6 e I11 na tabela de corrigidos.
- Seção 8: conclusões e próximos passos (3D com os testes como especificação executável, I7, mapa de origem, operadores,
  I9/I12, GeMA). Apêndice com as suítes novas, o código de apoio e o `stress.py` (sorteios pulados após aborto).
- Figuras regeneradas: todas iguais às da v0.4, mais `border_before`/`border_after` (no lugar de `border_fail`).
  Resumo, nota de rodapé (commit `97dcfb7`, com os testes da v0.5) e tabela de histórico atualizados.

### Modificações de código entre a v0.4 e a v0.5

- Testes automatizados novos, pedidos pelo autor ("todo princípio do projeto que for testável vale a pena um teste";
  validar também as estruturas internas do construtor):
  - `tests/support/InsertionChecks.{h,cpp}`: a topologia esperada (oráculo), calculada só da malha de entrada e das
    arestas (uma cópia por componente da estrela cortada); a verificação do resultado (partição dos cantos, número de
    pontos, um elemento de interface por aresta entre `a` e `b`, número de arestas colapsadas); o estado de cada vértice
    (máquina de estados da seção 3.4, contando os lados curtos: colapsado / colados / borda); o estado interno do
    construtor (flags de elementos e vértices e seus tamanhos, mapa `_collapsed`, aresta colapsada sempre lado curto de
    vértice interno em um só elemento de interface).
  - `tests/support/RandomMeshes.{h,cpp}`: gerador de malhas aleatórias em C++ (os 4 tipos do `stress.py` e malhas com
    furos, validadas), com `std::mt19937` e sorteio/embaralhamento próprios (mesmas malhas em qualquer compilador).
  - `BuilderInspector`: tamanhos dos vetores de flags e `insertEdge`, réplica da inserção que pode trocar Split Element
    por Expand Edge (auto-validada contra o construtor real).
  - `test_random_meshes.cpp` (10 testes, 5 tipos × 2): seleções aleatórias inseridas uma por chamada, em lote e em lotes
    com entradas rejeitadas; depois de cada chamada, V1–V7, contagem (e que contar não altera a malha), topologia
    esperada, estado dos vértices, estado do construtor e memória exata (capacidade = tamanho); também todas as arestas
    internas abertas.
  - `test_special_meshes.cpp` (4): malha sem aresta interna, dois triângulos separados, todas as arestas da malha base,
    pontos com 3 coordenadas.
  - `test_algorithm_structure.cpp` (30, 5 tipos × 6): gabarito do elemento (P1), decisão pelo estado do vértice
    calculada sem o construtor (P2), contrato de cada operador (Split move exatamente 2 cantos), localidade (P3),
    independência das coordenadas (P4), equivalência Split/Expand.
  - `test_base_mesh.cpp`: estado do construtor a cada passo e topologia esperada no fim.
  - Teste de mutação (no scratchpad): o Canônico tratando borda como interna, com a contagem ajustada, passa na rodada 2
    do `stress.py` (400/400) e falha em só 44/400 da rodada 1; 39 testes do Google Test falham (os 10 aleatórios, 13 de
    estrutura e 16 manuais).
  - `ctest`: 91 testes (89 do Google Test + 2 de estresse), todos ok em Debug e Release.
- `gen.py`: a figura do vértice de borda com duas arestas virou `border_before` / `border_after` (antes `border_fail`).

- B10 tratado com asserções, por decisão do autor (provisória: prefere que o erro apareça cedo; o relato de falhas fica
  para depois): em `insertInterfaceElements`, o resultado de cada operador (em A e em B) é guardado e verificado com
  `assert`; o operador `UNDEFINED` também para na asserção (antes, imprimia uma mensagem e seguia, deixando o elemento
  pela metade). Variáveis com `[[maybe_unused]]`, sem aviso com `NDEBUG`. O `@return` dos quatro operadores documenta
  que hoje sempre devolvem `true` e que o chamador verifica. `ctest`: 47 testes ok em Debug e Release.
- Mensagens de rejeição com o espaço que faltava (`[%u] is not valid`), corrigidas pelo autor.
- I11 corrigido (`IBHM.{h,cpp}`, `IBHMInterfaceElementBuilder.{h,cpp}`), opção B + complemento C, aprovada pelo autor:
  - `reserveSpaceForElements(p, e)`: garante `p` posições de half-edge e `e` entradas de elemento livres depois dos
    elementos válidos. As posições já reservadas e não usadas (de arestas rejeitadas) contam: só o que falta é
    acrescentado (antes, cada aresta rejeitada deixava 4 posições e uma entrada sobrando para sempre). Cada vetor cresce
    com `reserve` exato antes do `resize` (antes, `resize` dobrava a capacidade no libc++/libstdc++ e a multiplicava
    por 1,5 no MSVC).
  - Novo `reserveCapacityForElements(p, e)`: só capacidade, exata (par de `reserveSpaceForNodes` para os elementos).
    Com ela, `reserveSpaceForElements` não realoca enquanto a capacidade bastar.
  - Construtor: os vetores de flags (`_isInterfaceElement`, `_inInterfaceElement`) seguem a mesma política (exato, só o
    que falta, contando a partir do número de elementos e de pontos válidos).
  - Documentação: `reserveSpaceForElements`, `reserveCapacityForElements`, `numberReservedElements` (que confundia
    tamanho com capacidade) e `insertInterfaceElements` (comportamento de memória e como reservar antes).
  - Medição (grade 1000×1000 quadriláteros, 1M elementos, 4M half-edges, Apple clang -O2; ferramenta no scratchpad):
    V/O/Of alocados ao inserir 1 aresta: 68,7 MB antes, 34,3 MB agora (o usado é 34,3 MB). 1000 chamadas de 1 aresta
    sem reserva: antes 1 realocação e ~113 ms (com 2× de memória); agora 1000 realocações e ~700 ms (memória exata).
    Com `reserveCapacityForElements` + `reserveSpaceForNodes` antes: 0 realocações, ~26 ms e memória exata.
  - Testes novos: `ReserveSpaceForElementsGrowsExactly`, `ReserveSpaceForElementsOnlyAddsWhatIsMissing`,
    `ReserveCapacityForElementsKeepsTheSizeAndAvoidsReallocations` (`test_ibhm.cpp`),
    `RejectedEdgesDoNotAccumulateReservedSpace` (`test_invalid_input.cpp`) e `PreReservedCapacityAvoidsReallocations`
    (`test_base_mesh.cpp`: o exemplo inteiro, uma aresta por chamada, sem nenhuma realocação). `ctest`: 47 testes ok
    em Debug e Release; sem aviso novo com `NDEBUG`.
- I2 corrigido (`IBHM.{h,cpp}`, `isBorder`), como proposto e aprovado pelo autor:
  - O passo pela aresta colapsada (que vai do vértice para ele mesmo e não tem oposta) virou um passo normal da
    rotação: o próximo canto é `next(c)`, comparado com a half-edge inicial pela condição do laço como todos os outros.
    Antes, o código ia de `c` para `next(c)` e já calculava o passo seguinte, pulando a comparação: começando em
    `next(c)`, a rotação nunca parava. Fora esse caso, visita os mesmos cantos na mesma ordem; não há verificação nova
    (o ramo da colapsada faz até uma consulta `heOpposite` a menos).
  - Documentação do método no cabeçalho: qualquer half-edge do vértice serve, inclusive de elemento de interface; a
    borda de um buraco (inclusive o aberto pelo Open Hole) conta como borda.
  - Testes (em `test_ibhm.cpp`, suíte `IBHMIsBorder`): começar na aresta colapsada (51) e logo depois dela (48) após
    abrir K–L (era o teste desativado do I2); e `isBorder` comparado, para todas as half-edges, com o esperado calculado
    sobre a cópia da malha (o vértice tem uma half-edge de saída sem oposta), na malha de entrada e depois de cada uma
    das 12 inserções do exemplo. Com o `isBorder` anterior, os dois testes não terminam.
  - `tests/test_known_defects.cpp` removido (o único teste foi para `test_ibhm.cpp`) e retirado do CMake; o comentário
    do `tests/CMakeLists.txt` mantém o mecanismo de testes desativados para defeitos futuros.
  - Verificação: `ctest` com 42 testes ok em Debug e Release, nenhum desativado; estresse com os mesmos números
    (400/400 nas duas rodadas). Compila sem aviso novo com `NDEBUG`.
- I6 corrigido com asserções (pedido do autor), sem custo em release:
  - `IBHM::heNext`, `hePrevious` e `heElement`: `assert(halfEdge < numberOfHalfEdges())`. Os sentinelas `BORDER` e
    `COLLAPSED` e as posições reservadas ainda não validadas param na asserção, em vez de chegar à busca binária (antes,
    `heNext(BORDER)` só abortava quando não havia espaço reservado; com reserva, lia o offset reservado e devolvia lixo).
    Documentado como pré-condição no cabeçalho.
  - `IBHMInterfaceElementBuilder`: `assert(x1 != COLLAPSED)` e `assert(x3 != COLLAPSED)` em `retrieveAvailableVertex`
    (`he` e `b` estão em elementos contínuos, cujas arestas nunca são colapsadas); `assert(currentHE != COLLAPSED)` em
    `reindexElementsCCW` (pré-condição B8, agora verificada); asserções equivalentes nos dois laços do `openHole`. Cada
    uma com o motivo de valer pela arquitetura.
  - Teste de morte novo `IBHMHalfEdgesDeathTest.QueriesRejectSentinelsAndReservedPositions` (`test_ibhm.cpp`): `BORDER`,
    `COLLAPSED` e a posição reservada 48 nas três consultas. Com a IBHM anterior, o teste falha (as consultas não
    abortam). `ctest`: 40 testes ok em Debug e Release (o estresse roda com as asserções ligadas: nenhuma chamada
    legítima viola a pré-condição nas 800 malhas), 1 desativado (I2).
  - Correção de build: o commit `3b14af8` tinha trocado `if (foundCounterclockwise)`/`if (foundClockwise)` por
    `if (currentHE != IBHM::BORDER)` no `openHole`, mas a `assert(foundCounterclockwise || foundClockwise)` ficou, sem as
    variáveis: não compilava com as asserções ligadas (Debug e testes). As variáveis voltaram, a pedido do autor, com
    os nomes `foundCCW` e `foundCW`, usadas nos dois `if` e na asserção (assim não geram aviso de variável não usada
    com `NDEBUG`).
- I1 corrigido (`IBHMInterfaceElementBuilder.{h,cpp}`, `openHole`), como proposto e discutido com o autor:
  - Decisão do autor: tratar o caso dentro do Open Hole (sem quinto operador). As regras de decisão levam a ele: o
    vértice de borda já aberto está em elemento de interface e não tem aresta colapsada. Continua sendo um buraco,
    aberto para o exterior; o lado que chega à borda faz o mesmo que o Canônico de borda.
  - Laço 1 (anti-horário): se chega à borda, o setor entre a aresta nova e a borda já está com o vértice novo e não há
    canto de elemento de interface a mover nem lado curto a descolar (a `assert(currentHE != BORDER)` virou `if`).
  - Laço 2 (horário): testa `O[h] == BORDER` *antes* de aplicar `next` (antes aplicava `next(BORDER)`, o que na IBHM é
    uma busca binária com índice inválido); se chega à borda, não há o que descolar. Resolve também a parte do I6 que
    cabia ao segundo laço do Open Hole.
  - Asserção única depois dos dois laços (pedida pelo autor): ao menos um lado encontra elemento de interface, porque o
    vértice pertence a um. Só em debug; em release não custa nada.
  - Custo: o laço 1 não muda; no laço 2, o teste inútil depois do `next` foi trocado pelo teste antes dele (mesmo número
    de comparações). Nenhuma rotação nova, nenhum `isBorder`.
  - Comentários detalhados no código e na documentação do operador (`openHole` e `OPEN_HOLE` no cabeçalho), e no
    comentário da contagem (`computeNumberOfNewVertices`), que dizia que o caso não era suportado.
  - Testes: nova suíte `tests/test_open_hole_border.cpp` (6 testes): a regra escolhe Open Hole na segunda aresta; leque
    nas duas ordens e em lote (cada ordem leva um laço diferente à borda); vértice H da malha base (H–M e G–H nas duas
    ordens e em lote); exemplo dos slides seguido de G–H; vértice 4 da malha com furo (`meshIBHM.txt`); vértice de
    borda com 3 arestas em todas as 6 ordens e em lote (aresta do meio por último: os dois laços encontram elemento de
    interface). Todos verificam V1–V7, a contagem, o número de cópias (k+1) e a mesma topologia. `TestMeshes` ganhou
    `wideFanMesh()` e `holeMesh()`. Os dois testes desativados do I1 saíram de `test_known_defects.cpp` (fica só o I2);
    `VertexCountFan` passou a inserir também. `stress_all` reativado no CTest.
  - Verificação: `ctest` com 39 testes ok (Debug e Release), 1 desativado (I2). Rodada 1 do estresse (seed 21): 400/400,
    148 casos com vértice de borda com 2+ arestas, sem dependência de ordem ou de lote e contagem exata; rodada 2: mesmos
    números de antes. Controle negativo: com o `openHole` anterior, 6 dos testes novos abortam na asserção do laço 1.
  - `stress.py`: docstring atualizada (I1 corrigido; com aborto, os sorteios restantes do caso são pulados e os casos
    seguintes mudam, por isso 141 → 148). `gen.py`: comentário da figura `border_fail` atualizado.

## v0.4 — 19/10/2026

- Reflete as correções feitas desde a v0.3 (lista abaixo): seção 2 (`numberOfHalfEdges()`, `duplicatePoint()`,
  `O[]` reservado começando com `BORDER`, passo a passo da inserção), seção 3 (decisões puramente topológicas, cópias
  herdam as coordenadas; `BuilderInspector` como classe amiga), seção 4 (pseudocódigos com `duplicatePoint(v)`;
  contagem prévia reescrita em duas etapas, exata), seção 5 (teste do índice no Algoritmo de travessia, rejeições,
  memória exata, coordenadas das cópias).
- Seção 6: verificação V7; rodadas do estresse refeitas com o código desta versão (mesmos números: 259/141 e 400/400);
  experimento Split → Expand refeito (791 ocorrências, topologia idêntica nas 1600 execuções); tabela de cenários
  atualizada (marcados com † os que mudaram); nova seção 6.3 de testes automatizados, com a tabela das suítes e o
  controle negativo contra o núcleo antes do I3, do I5 e do I4.
- Seção 7: B7 e B9 atualizados; tabela de defeitos dividida em "em aberto" (I1, I2 latente, I6, I7, I9, I11) e
  "corrigidos desde a v0.3" (I3, I4, I5, I8, I10, com a correção e os testes de cada um).
- Seção 8: próximos passos atualizados (I1 primeiro e reativação dos testes desativados; ampliar testes; mapa vértice
  novo → origem).
- Apêndice A reescrito: testes automatizados (`tests/`, Google Test + CTest, asserções ligadas, código de apoio,
  testes desativados), build com CMake e direto do `driver`, `driver` sobre o código compartilhado (saída idêntica),
  código de saída do `stress.py`; saiu a seção do `test_coordinates.cpp`.
- Resumo, nota de rodapé (commit `f4a5a83`) e tabela de histórico atualizados. Figuras regeneradas com o `driver` atual:
  idênticas às da v0.3. Docstring do `gen.py` corrigida (cópias com as coordenadas do original, V7).

### Modificações de código entre a v0.3 e a v0.4

- I4 corrigido (`IBHM.{h,cpp}`, `IBHMInterfaceElementBuilder.cpp`): novo método `IBHM::duplicatePoint(vertex)`, que
  acrescenta um ponto com as coordenadas de um vértice existente. Lê a origem por índice e copia cada valor para uma
  variável local antes do `push_back`, então não precisa de vetor temporário e continua correto se o vetor for
  realocado. Os quatro operadores (Canônico de borda, Split, Expand, Open Hole) usam `duplicatePoint(v)` em vez de
  `addPoint()`: as cópias passam a nascer na posição do vértice original. Escolhido em vez de montar um
  `std::vector` com as coordenadas a cada cópia, para não alocar memória temporária no caminho crítico.
  - Ferramentas: o `driver` ganhou a verificação **V7** (toda cópia tem as coordenadas do vértice original);
    `test_coordinates.cpp` ganhou 3 verificações de `duplicatePoint` (dentro da reserva, com realocação durante a cópia
    e em 3D), total de 13.
  - Verificação: teste de coordenadas 13/13; exemplo dos slides com a mesma topologia (incremental e lote) e V7 ok;
    rodadas do estresse com os mesmos números (259/141 e 400/400), agora também com V7 em todos os casos válidos.
    Controle negativo: com o construtor antigo, a V7 falha já na primeira aresta do exemplo.
- I8 corrigido (pelo autor, revisado): as mensagens de operador indefinido em `insertInterfaceElements` usam `%u`, e
  a do vértice B imprime `b` em vez de `a`. Compila sem avisos; exemplo e 100 casos do estresse sem alteração.
- I10, parte dos includes, corrigida: `IBHM.cpp` inclui `<cassert>`, `<cstdio>` (`printf` em `print()`), `<cstring>`
  (`memset`) e `<algorithm>` (`std::upper_bound`); `IBHMInterfaceElementBuilder.cpp` inclui `<cassert>` e `<cstdio>`
  (`printf`). Saíram os includes sem uso (`<map>` em `IBHM.cpp` e `<iostream>` no construtor). Compila sem avisos;
  exemplo, teste de coordenadas e 100 casos do estresse sem alteração. A parte do `CMakeLists.txt` (exige Qt) continua
  em aberto.
- Convenção definida pelo autor: funções e tipos herdados de C (`printf`, `memset`, `size_t`) são usados sem `std::`,
  com `<cstdio>` e `<cstring>`. (Uma versão intermediária com `std::` foi descartada.) A inspeção do CLion pode marcar
  `<cstdio>`/`<cstring>` como desnecessários por inclusão indireta no libc++; o `misc-include-cleaner` do clang-tidy os
  considera necessários, e eles são mantidos.
- I5 corrigido (`IBHMInterfaceElementBuilder.{h,cpp}`): `computeNumberOfNewVertices` agora devolve o número **exato**
  de vértices novos. Em duas etapas: (1) seleciona só as arestas que `insertInterfaceElements` vai abrir, com os mesmos
  testes (pula arestas de borda, de elementos de interface e repetidas, identificadas por `min(a, O[a])`) e conta, por
  vértice, quantas o tocam; (2) aplica a regra uma vez por vértice: em elemento de interface → n; fora, na borda → n;
  fora, interno → n − 1. O teste de borda roda uma vez por vértice distinto (antes: duas vezes por aresta). Código e
  cabeçalho comentados em detalhe.
  - Efeito colateral: como as arestas de elementos de interface são descartadas antes de `isBorder`, a entrada que
    travava o programa (I2, ex.: `34` e depois `48`) agora é rejeitada normalmente. O defeito latente de `isBorder` (o
    desvio da aresta colapsada pula a parada) continua, mas o construtor não chega mais a esse caminho.
  - Novo teste `doc/report/tools/test_vertex_count.cpp` (8 casos: exemplo em lote, aresta repetida inclusive pela outra
    meia-aresta, aresta de borda, combinação, vértice já em elemento de interface, aresta de elemento de interface e
    vértice de borda com duas arestas). Código novo: 8/8. Código antigo: 5 contagens erradas e trava no caso da aresta
    de elemento de interface.
  - Estresse: mesmos números (259/141 e 400/400) e nenhuma contagem diferente da criada, inclusive no lote.
- I3 corrigido (`IBHM.{h,cpp}`, `IBHMInterfaceElementBuilder.{h,cpp}`), como sugerido na seção 7.4:
  - Novo `IBHM::numberOfHalfEdges()` (= `Of[n_c]`, o número de half-edges dos elementos válidos; ao contrário de
    `numberReservedElements()`, não conta o espaço reservado).
  - `reserveSpaceForElements` inicializa as posições reservadas de `O[]` com `BORDER` (antes, 0, que é uma half-edge
    válida). Os operadores sempre definem a oposta das quatro half-edges novas, então a inserção não muda.
  - `insertInterfaceElements` lê o limite `numberOfHalfEdges()` **no início da chamada** e rejeita, com mensagem no
    console como as outras rejeições, todo índice `>=` limite (fora do vetor ou no espaço reservado). É o primeiro
    teste, antes de `heOpposite`. `computeNumberOfNewVertices` aplica o mesmo teste (teste 0), então a contagem
    continua exata. Um índice de elemento de interface criado na própria chamada também fica fora do limite e é
    rejeitado; criado em chamada anterior, continua rejeitado pelo teste de elemento de interface.
  - Custo: uma comparação por aresta da entrada, fora dos operadores.
  - O `@warning` de `insertInterfaceElements` (índices não validados e laço infinito, este já resolvido pelo I5) foi
    substituído pela lista de rejeições, que agora inclui os índices fora da malha.
  - Ferramentas: o `driver` não consulta mais índices fora da malha antes de passá-los ao construtor (imprime os
    vértices como `BORDER`); `test_vertex_count.cpp` ganhou 3 casos (índice fora do vetor, índice no espaço reservado
    com verificação de `O[]` = `BORDER`, e índices inválidos misturados a uma aresta válida no mesmo lote): 11 casos,
    15 verificações, todas ok.
  - Controle com o código antigo: `999` para na asserção de `heOpposite`; `48` no espaço reservado (após uma chamada
    só com aresta de borda) é contado como 2 vértices novos e a inserção para na asserção de `openHole`. Código novo:
    ambos rejeitados, nenhum vértice ou elemento criado.
  - Verificação: exemplo dos slides com 19 vértices novos, mesma partição incremental e em lote; estresse com os
    mesmos números (259/141, abortos todos I1, e 400/400) e `prediction_mismatch` 0.
- I10 concluído (`CMakeLists.txt`): o CMake não depende mais de Qt (saíram `find_package(Qt5/Qt6)`, `AUTOMOC/UIC/RCC`,
  a ligação com Qt e o ícone de Windows inexistente). `InterfaceElementsLib` (núcleo) tem includes `PUBLIC`; o
  executável `InterfaceElementsPrototype` liga com ela e compila só `main.cpp` e o CHE legado. Mínimo do CMake: 3.16.
- Testes automatizados com Google Test + CTest (`tests/`, opção `INTERFACE_ELEMENTS_BUILD_TESTS`, ligada por padrão):
  - Google Test: pacote instalado (`find_package(GTest CONFIG)`) ou v1.17.0 baixado pelo `FetchContent` (URL do
    GitHub com SHA256 fixo). Cada `TEST` vira um teste do CTest em processo próprio (`gtest_discover_tests`, timeout
    60 s), então um `assert` ou um laço infinito falha só aquele teste.
  - Os testes dependem dos `assert`: o núcleo é recompilado em `tests/` (`InterfaceElementsLibChecked`) sem `NDEBUG`,
    em qualquer tipo de build (conferido no Release: `InterfaceElementsLib` com `NDEBUG`, testes e driver sem).
  - `tests/support/`: `TestMeshes` (malha base, leque, dois triângulos), `MeshValidation` (cópia da malha pela
    interface pública, V1–V5, origem das cópias e V7, partição dos cantos de `compare.py`) e `BuilderInspector`
    (acesso de leitura ao estado privado; o construtor ganhou só a linha `friend class BuilderInspector;`, sem custo).
  - Suítes: `test_ibhm` (13: pontos, reserva exata, `duplicatePoint` com realocação, 3D, teste de morte do
    `addPoint` com tamanho errado, `numberOfHalfEdges`, `O[]` reservado = `BORDER`), `test_vertex_count` (6, os casos
    1–8 do teste antigo), `test_base_mesh` (5: os 24 operadores e os vértices novos de cada uma das 12 inserções do
    exemplo, com V1–V7 a cada passo; malha final; lote = incremental; duas chamadas = uma; 21 ordens com a meia-aresta
    sorteada), `test_invalid_input` (7: I3 e rejeições sem alterar a malha), `test_known_defects` (3 desativados:
    I1 incremental e em lote, I2 em `isBorder`; rodados à força, I1 aborta em `openHole` e I2 não termina).
  - CTest: 31 testes de unidade (~0,2 s) com o rótulo `unit`; `stress_supported` (seed 22/400 com
    `--skip-border-multi`, ~7 s) com o rótulo `stress`; `stress_all` (seed 21/400) registrado e desativado até o I1.
  - Controle negativo (mesmas suítes compiladas contra o núcleo antigo): antes do I3 (`3b991e3`), 4 falhas; antes
    do I5 (`2a4eb0e`), 12 falhas, 3 delas por travamento; antes do I4 (`28dd85d`, sem `test_ibhm`), 15 falhas,
    inclusive a V7 no exemplo. Núcleo atual: todas passam.
- Driver (`tools/driver.cpp`): as verificações V1–V5 e V7 e a cópia da malha vêm de `tests/support/MeshValidation`, e
  o estado privado do construtor é lido por `BuilderInspector`; saiu o `#define private public` (que no MSVC muda os
  nomes decorados e quebra a ligação). Saída idêntica à anterior: console e JSON do exemplo, do lote, dos casos do
  I3 e dos 474 arquivos de 120 casos de cada rodada do estresse. Também é o alvo `driver` do CMake.
- `stress.py` devolve código de saída 1 se houver aborto, caso inválido, dependência de ordem ou de lote, ou contagem
  errada (usado pelo CTest).
- Removidos `tools/test_coordinates.cpp` e `tools/test_vertex_count.cpp`, substituídos pelas suítes em `tests/`.

## v0.3 — 18/10/2026

- Saiu o confronto com a apresentação de projeto (antiga seção 7.1, itens A1–A10) e as referências a ele; a
  apresentação não é mais mantida e continua citada só como origem das ideias e do exemplo. Com isso, a seção de
  defeitos passou de 7.5 para **7.4**.
- Nova figura dedicada às hipóteses x1 e x3 do Split Element (seção 4.2), com as consultas de
  `retrieveAvailableVertex` e o caso em que nenhuma vale (Expand Edge).
- Seção 7.1 (comentários) virou uma tabela com o estado de B1–B14: todos corrigidos, exceto o B10. Seção 7.4
  (defeitos) atualizada: I1 com as duas asserções observadas, I4 restrito às cópias na origem, novo I11 (política de
  memória em `reserveSpaceForElements` e nos vetores do construtor).
- Seção 2 atualizada com a reserva exata e o novo `addPoint`; verificação com o teste de coordenadas e a contagem das
  asserções da rodada 1.
- Ferramentas documentadas (`driver.cpp`, `gen.py`, `stress.py`, `compare.py`) e novo `test_coordinates.cpp`; o
  apêndice A explica cada uma, o formato do JSON e o desenho das figuras, com uma figura nova das malhas aleatórias.
- Verificação refeita com o código do commit `066cbde`: mesmos números da v0.2 (rodada 1: 259 válidas e 141 abortos,
  todos com vértice de borda com 2+ arestas; rodada 2: 400/400; Split → Expand: 791 ocorrências, topologia idêntica).

### Modificações de código entre a v0.2 e a v0.3

- B1 corrigido: em `IBHMInterfaceElementBuilder.h`, o `@param he` de `openHole` agora diz que o vértice já pertence a
  um elemento de interface e **não** tem aresta colapsada (não está em `_collapsed`). No relatório: retirar B1 da lista
  da seção 7.2 (ou marcá-lo como resolvido).
- B3 corrigido: em `IBHM.h`, o comentário de `_halfEdgeVertex` não cita mais `_numberVerticesByElement` (membro da CHE);
  descreve a organização por `_offset` (elemento i nas posições `_offset[i]` a `_offset[i+1]-1`) e o espaço reservado.
- B4 corrigido: na documentação da classe `IBHMInterfaceElementBuilder`, "There are only 4 operators to 3D" virou
  "only 4 operators in 2D ... topological only ... to ease the future extension to 3D".
- B5 corrigido: `splitElement` descreve as duas posições do elemento compartilhado (elemento de `he` ou de
  `opposite(he)`), correspondentes às hipóteses x1 e x3.
- B6 corrigido: `heOpposite`, `_oppositeHalfEdge` e o novo comentário do enum `OPPOSITE` documentam `BORDER (-1)` e
  `COLLAPSED (-2)`.
- B7 corrigido (só documentação): `computeNumberOfNewVertices` descreve as regras e diz que o resultado é um limite
  superior, explicando as duas fontes de excesso (arestas que serão rejeitadas e a regra de borda aplicada por aresta,
  que conta 2k - 1 em vez de k). O comportamento não mudou; a contagem em excesso continua sendo o defeito I5.
- B8 corrigido: `reindexElementsCCW` documenta a pré-condição de não chegar à half-edge que segue uma aresta colapsada.
- B9 corrigido: `insertInterfaceElements` documenta as arestas ignoradas (borda, já em elemento de interface,
  repetidas), que nada é informado a quem chama, e um `@warning` sobre índices não validados (I2/I3). Corrigido também
  "There user can select anyone of them".
- B11 corrigido (só documentação): `numberReservedElements` agora diz que devolve o número de posições de half-edge
  reservadas, não de elementos. O nome do método não mudou (não é usado em lugar nenhum).
- B12 corrigido: "See @reindexElements" virou "See reindexElementsCCW()".
- B13 corrigido: `_collapsed` diz que o valor é a própria half-edge colapsada (h3 ou h1) e quando a entrada é removida.
- B14 corrigido: cabeçalho de `IBHM.h` ("File: IBHM.h"), `elementsList` (sem citar a CHE), destrutor ("Default
  destructor"), parâmetro inexistente `numberOfVertices` no construtor, descrição de `buildOppositesLinear`, "the
  bother" → "the border" e "the same build" → "the same builder".
- B10 **continua em aberto**. Uma solução foi implementada e depois revertida a pedido do autor, por acrescentar
  verificações ao caminho crítico e deixar o algoritmo mais complexo e possivelmente mais lento. Para retomar no futuro,
  a ideia era: cada operador verifica, antes de alterar a malha, se pode ser aplicado e devolve `false` sem mudar nada
  (Canônico: vértice já em elemento de interface; Split: sem aresta colapsada ou meias-arestas fora das hipóteses x1/x3;
  Expand: rotação a partir de `c` que não alcança `he`; Open Hole: uma das rotações chega à borda antes de achar um
  elemento de interface); um método `applyOperator(he, elementHE)` substitui os dois `switch`, e `insertInterfaceElements`
  trata o retorno com `assert`. Com ela, os 141 abortos da rodada 1 passavam a vir de `assert(appliedA/appliedB)` e, em
  release, não havia mais escrita fora do vetor. Princípio definido pelo autor: a arquitetura deve garantir que o
  operador escolhido sempre seja aplicável; caso contrário, é bug.
- B2 corrigido e I4 corrigido em parte (código, `IBHM.{h,cpp}`):
  - `reserveSpaceForNodes` usa `reserve` **exato** (só a capacidade cresce, exatamente o pedido); o tamanho de
    `_coordinates` passa a ser sempre `numberPoints() * dim`. Decisão do autor: eficiência de memória é prioridade, sem
    crescimento geométrico; se forem esperadas muitas inserções, o usuário reserva mais espaço de uma vez.
  - `addPoint` recebe `const std::vector<float> &coordinates = {}` em vez de `const float*`. Não há limite fixo para o
    número de coordenadas (decisão do autor: preferir uma estrutura flexível). O vetor deve ter exatamente
    `numberCoordinatesByVertex()` valores, ou ser vazio para usar a origem (padrão). Como é lido pelos próprios índices,
    e não por um ponteiro para o buffer interno, é seguro montá-lo a partir das coordenadas de um ponto da malha. O
    construtor verifica `numberCoordinates > 0`. (Uma versão intermediária com `std::array` de tamanho fixo foi
    descartada.)
  - Comentários de `addPoint`, `reserveSpaceForNodes` e `_numberOfValidPoints` atualizados.
  - Verificação: teste de coordenadas (reserva exata, índice correto, cópia de vértice, sem realocação dentro da reserva,
    crescimento além da reserva, malha 3D com 3 coordenadas), exemplo dos slides (incremental e lote: mesma topologia da
    v0.2) e as duas rodadas do estresse com números idênticos aos da v0.2 (259/141 e 400/400).
  - Falta em I4: as cópias criadas pelos operadores ainda nascem na origem (o construtor chama `addPoint()` sem
    coordenadas).

### Observações registradas para o texto da v0.3

- I1 (antes da correção do B10) aparecia em modo debug por duas asserções: `currentHE != IBHM::BORDER` em `openHole`
  (72 das 141 malhas da rodada 1) e `position < _offset.size() - 1` em `heNext`, chamada pelo segundo laço do
  `openHole` com `BORDER` (69). (Com a solução do B10, revertida, passava a aparecer como `assert(appliedA/appliedB)`.)
- `reserveSpaceForElements` continua usando `resize` (necessário, pois as posições são gravadas por índice), mas o
  `resize` cresce geometricamente e pode alocar até o dobro do necessário em `_halfEdgeVertex`, `_oppositeHalfEdge` e
  `_offset` (o mesmo vale para os vetores `_isInterfaceElement`/`_inInterfaceElement` do construtor). Se a política de
  memória exata valer também aqui, basta chamar `reserve` exato antes do `resize`. Ainda não decidido pelo autor.


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
