dune_add_test(SOURCES globalindexsettest.cc)

dune_add_test(SOURCES persistentcontainertest.cc)

dune_add_test(SOURCES structuredgridfactorytest.cc
              LINK_LIBRARIES dunegrid)

dune_add_test(SOURCES tensorgridfactorytest.cc
              LINK_LIBRARIES dunegrid)

dune_add_test(SOURCES vertexordertest.cc
              LINK_LIBRARIES dunegrid)
