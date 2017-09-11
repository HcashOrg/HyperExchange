
Embedded genesis
----------------

Compile executable which references a specific genesis state.  The
full genesis state can be included by linking with `egensis_full`,
or only hashes (chain ID and hash of JSON) can be included by linking
with `egenesis_brief`.

The `GRAPHENE_EGENESIS_JSON` parameter specifies the `genesis.json`
to be included.  Note, you will have to delete your `cmake` leftovers
with

    make clean
    rm -f CMakeCache.txt
    find . -name CMakeFiles | xargs rm -Rf

The embedded data can be accessed by functions in `egenesis.hpp`.

Note, if your `genesis.json` contains newlines, you should be aware there are [newline translation issues](https://github.com/cryptonomex/graphene/issues/545) and should keep in mind this advice:

- If you're creating a new chain, run your genesis file through `canonical_format.py` before publishing it.  It should get rid of line translation issues and has the added benefit of making the file smaller.
- If you're creating a new chain, and you want to show a pretty genesis file with newlines and whitespace in your Github, you can create the ugly file by running `canonical_format.py` as part of the build process by editing `CMakeLists.txt`.  If you do this, your build will then depend on having a working Python installation (which is why we don't do this).
- If you have an existing chain with a pretty genesis, or want to disregard the above advice and create a new chain with a pretty genesis, and you're using Git to distribute your genesis file, you should add a `.gitattributes` file to your repository as discussed [here](https://help.github.com/articles/dealing-with-line-endings/).