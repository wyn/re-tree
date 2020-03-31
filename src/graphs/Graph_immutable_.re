module I = Identity;
module ID = I.FocusId;
module PID = I.ParentId;
module CID = I.ChildId;
module P = Path.T;
module IDTree = IDTree.T;

module Make(S:Data.STRINGLY) = {

type el = S.t;
type err = string;

type dataWithPath = {
  pathUp: P.t,
  value: el
};

type t = {
  masterLookup: ID.Map.t(dataWithPath),
  tree: IDTree.t,
};

let empty = (): t => {
  let tree = IDTree.empty();
  let masterLookup = ID.Map.make();
  {masterLookup, tree};
};

let toString = (graph) => {
  let treeS = graph.tree->IDTree.toString;
  let dataS =
    graph.masterLookup
    ->Map.reduce(
        [],
        (acc, ky, d: dataWithPath) => {
          let s =
            ky->ID.toString
            ++ ": { "
            ++ "value: "
            ++ d.value->S.toString
            ++ ", "
            ++ "pathUp: "
            ++ d.pathUp->P.toString
            ++ " }";
          [s, ...acc];
        },
      )
    |> String.concat("\n");
  "{tree:\n" ++ treeS ++ "\n masterLookup:\n" ++ dataS ++ "\n}";
};


let size = (graph: t) => {
  graph.masterLookup->Map.size;
};

let hasChildren = graph => {
  graph.tree->IDTree.hasChildren;
};

let numberChildren = graph => {
  graph.tree->IDTree.children->Map.keysToArray->Array.size;
};

let containsId = (graph: t, id: ID.t): bool => {
  graph.masterLookup->Map.has(id);
};

let fullDataFromNode = (graph: t, id: ID.t): option(dataWithPath) =>
  if (graph->containsId(id)) {
    let dataWithPath = graph.masterLookup->Map.getExn(id);
    Some(dataWithPath);
  } else {
    None;
  };

let pathFromNode = (graph: t, id: ID.t): option(P.t) => {
  graph->fullDataFromNode(id)->Option.map(d => d.pathUp);
};

let dataForNode = (graph: t, id: ID.t): option(el) =>
  graph->fullDataFromNode(id)->Option.map(d => d.value);

let depth = (graph: t, id: ID.t): int => {
  graph->pathFromNode(id)->Option.map(P.size)->Option.getWithDefault(0);
};

let maxDepth = (graph: t, _id: ID.t): int => {
  graph.masterLookup
  ->Map.reduce(0, (acc, _ky, dataWithPath) => {
      max(acc, dataWithPath.pathUp->P.size)
    });
};

let setDataForNode = (graph: t, id: ID.t, f: 'a => 'a): t =>
  switch (graph->fullDataFromNode(id)) {
  | Some(olddata) =>
    let masterLookup =
      graph.masterLookup
      ->Map.set(id, {...olddata, value: f(olddata.value)});
    {...graph, masterLookup};
  | None => graph
  };

let subGraphForNode = (graph: t, id: ID.t): option(t) => {
  switch (graph->pathFromNode(id)) {
  | Some(path) =>
    switch (graph.tree->IDTree.getSubtree(path, id)) {
    | Some(tree) =>
      /* [%log.debug */
      /*   "subgraphfornode tree: " ++ tree->IDTree.toString; */
      /*   ("", "") */
      /* ]; */
      let allIds = graph.masterLookup->Map.keysToArray; //;
      /* [%log.debug */
      /*   "all ids: " */
      /*   ++ ( */
      /*     allIds->Array.map(cid => cid->ID.toString)->List.fromArray */
      /*     |> String.concat(",") */
      /*   ); */
      /*   ("", "") */
      /* ]; */
      let ids = tree->IDTree.getAllIds;
      /* [%log.debug */
      /*   "to keep: " */
      /*   ++ ( */
      /*     ids->Array.map(cid => cid->CID.toString)->List.fromArray */
      /*     |> String.concat(",") */
      /*   ); */
      /*   ("", "") */
      /* ]; */
      let toKeep = ids->Array.map(I.convertChildToFocus)->ID.Set.fromArray;
      let cids = allIds->ID.Set.fromArray->Set.diff(toKeep)->Set.toArray;
      /* [%log.debug */
      /*   "to remove: " */
      /*   ++ ( */
      /*     cids->Array.map(cid => cid->ID.toString)->List.fromArray */
      /*     |> String.concat(",") */
      /*   ); */
      /*   ("", "") */
      /* ]; */
      // remove these from graph
      let masterLookup = graph.masterLookup->Map.removeMany(cids);
      let ret = {masterLookup, tree};
      /* [%log.debug */
      /*   "subGraphForNode returning: " ++ ret->toString(_ => ""); */
      /*   ("", "") */
      /* ]; */
      Some(ret);
    | None => None
    }

  | None => None
  };
};

let addNodeAtPath = (graph: t, id: ID.t, data: 'a, path: P.t): t => {
  let tree = graph.tree->IDTree.addChild(path, id);
  let masterLookup =
    graph.masterLookup->Map.set(id, {pathUp: path, value: data});
  {masterLookup, tree};
};

let addNode = (graph: t, id: ID.t, data: 'a): t => {
  let path = P.empty();
  graph->addNodeAtPath(id, data, path);
};

let addNodeUnder = (graph: t, id: ID.t, data: 'a, under: PID.t): t => {
  switch (graph->pathFromNode(under->I.convertParentToFocus)) {
  | Some(path) =>
    let path = path->P.append(under);
    graph->addNodeAtPath(id, data, path);
  | None => graph
  };
};

let removeNode = (graph: t, id: ID.t): Result.t(t, err) =>
  // if id exists then remove it from masterLookup and tree
  if (graph->containsId(id)) {
    // first need to find its path in the tree
    switch (graph->pathFromNode(id)) {
    | Some(path) =>
      /* %log.debug */
      /* "removeNode:" ++ id->ID.toString; */
      /* %log.debug */
      /* "removeNode - pathUp:" ++ path->P.toString; */
      let pid = id->I.convertFocusToParent;
      let masterLookup =
        graph.masterLookup
        ->Map.remove(id) // have to remove the node
        ->Map.map(dataWithPath => {
            {
              ...dataWithPath,
              // and update any paths that may have contained the removed node
              pathUp: dataWithPath.pathUp->P.removeElement(pid),
            }
          });
      let tree =
        graph.tree->IDTree.removeChild(path, id->I.convertFocusToChild);
      Result.Ok({masterLookup, tree});
    // can now remove from master list and tree
    | None => Result.Error("removeNode failed to get parent path")
    };
  } else {
    Result.Ok(graph);
  };

let moveChild =
    (graph: t, from: CID.t, under: PID.t): Result.t(t, err) => {
  let pid = under->I.convertParentToFocus;
  if (graph->containsId(pid)) {
    let id = from->I.convertChildToFocus;
    switch (graph->dataForNode(id)) {
    | Some(data) =>
      // remove old child node first as it might edit the graph
      // then can calculate new paths properly
      graph
      ->removeNode(id)
      ->Result.flatMap(graph => {
          // know pid already exist
          switch (graph->pathFromNode(pid)) {
          | Some(parentPathUp) =>
            // NOTE remember to append the new "under" id
            let pidPathUp = parentPathUp->P.append(under);
            /* %log.debug */
            /* "moveChild:" ++ pidPathUp->P.toString; */
            let masterLookup =
              graph.masterLookup
              ->Map.set(id, {pathUp: pidPathUp, value: data});

            let tree = graph.tree->IDTree.addChild(pidPathUp, id);
            Result.Ok({masterLookup, tree});
          | None => Result.Error("moveChild failed to get parent path")
          }
        })
    | None => Result.Error("moveChild failed to get data")
    };
  } else {
    /* %log.debug */
    /* "passthrough"; */
    Result.Ok(graph);
  };
};

let removeSubtree = (graph: t, id: ID.t): Result.t(t, err) =>
  // if id exists then remove it from masterLookup and tree
  if (graph->containsId(id)) {
    // first need to find its path in the tree
    switch (graph->pathFromNode(id)) {
    | Some(path) =>
      /* %log.debug */
      /* "found path: " ++ path->P.toString; */
      // NOTE remember to append id because do not want to delete siblings of id
      let idPath = path->P.append(id->I.convertFocusToParent);
      // can now remove id and all children from master list and tree
      let ids =
        graph.tree
        ->IDTree.getChildIds(idPath, false) // childIds doesnt include id
        ->Array.map(I.convertChildToFocus)
        ->Array.concat([|id|]); // so add it on explicitly
      /* %log.debug */
      /* ids->Array.map(ID.toString)->List.fromArray |> String.concat(","); */
      let masterLookup = graph.masterLookup->Map.removeMany(ids);
      // remove the subtree tipped by id found at path
      let tree =
        graph.tree->IDTree.removeSubtree(path, id->I.convertFocusToChild);
      Result.Ok({masterLookup, tree});
    | None => Result.Error("removeSubtree failed to get parent path")
    };
  } else {
    Result.Ok(graph);
  };

let setSubGraphForNode =
    (graph: t, id: ID.t, subgraph: t): Result.t(t, err) => {
  /* [%log.debug "setSubGraphForNode: " ++ id->ID.toString; ("", "")]; */
  /* [%log.debug "input graph:" ++ graph->toString(_d => "unknown"); ("", "")]; */
  /* [%log.debug */
  /*   "adding subgraph:" ++ subgraph->toString(_d => "unknown"); */
  /*   ("", "") */
  /* ]; */
  /* [%log.debug "under:" ++ id->ID.toString; ("", "")]; */
  let masterLookup = graph.masterLookup;
  switch (graph->pathFromNode(id)) {
  | Some(pathUp) =>
    /* [%log.debug "got pathUp: " ++ pathUp->P.toString; ("", "")]; */
    graph
    ->removeSubtree(id)
    ->Result.flatMap(graph => {
        /* [%log.debug "removed subtree at: " ++ id->ID.toString; ("", "")]; */
        /* [%log.debug graph->toString(_d => "unknown"); ("", "")]; */

        let tree = graph.tree->IDTree.addSubtree(id, pathUp, subgraph.tree);
        /* [%log.debug "got tree: " ++ tree->IDTree.toString; ("", "")]; */

        // know pid already exist
        let pidPathUp = pathUp->P.append(id->I.convertFocusToParent);
        /* [%log.debug "got pid pathUp: " ++ pidPathUp->P.toString; ("", "")]; */

        let toMerge =
          tree
          ->IDTree.getChildPaths(pidPathUp, true) // include the original parent ID in this
          ->Array.map(d => {
              let i = fst(d)->I.convertChildToFocus;
              let pth = snd(d);
              /* [%log.debug */
              /*   "got i: " ++ i->ID.toString ++ " - " ++ pth->P.toString; */
              /*   ("", "") */
              /* ]; */
              // if the subgraph lookup doesnt have the dataForNode
              // then hope it is on the original lookup
              switch (subgraph.masterLookup->Map.get(i)) {
              | Some(dataWithPath) => (i, {...dataWithPath, pathUp: pth})
              | None =>
                let dataWithPath = masterLookup->Map.getExn(i);
                (i, {...dataWithPath, pathUp: pth});
              };
            });

        let ret = {
          masterLookup: graph.masterLookup->Map.mergeMany(toMerge),
          tree,
        };
        /* [%log.debug */
        /*   "setSubGraphForNode returning: " ++ ret->toString(_ => ""); */
        /*   ("", "") */
        /* ]; */
        Result.Ok(ret);
      })
  | None =>
    [%log.debug "didn't get pathUp for id: " ++ id->ID.toString; ("", "")];
    Result.Ok(graph);
  };
};

let moveSubtree = (graph: t, from: CID.t, under: PID.t) => {
  let id = from->Identity.convertChildToFocus;
  let pid = under->I.convertParentToFocus;
  let masterLookup = graph.masterLookup;
  switch (graph->containsId(id), graph->containsId(pid)) {
  | (true, true) =>
    /* %log.debug */
    /* "pidPath In: " ++ graph->pathFromNode(pid)->Option.getExn->P.toString; */
    switch (graph->subGraphForNode(id)) {
    | Some(subtree) =>
      graph
      ->removeSubtree(id)
      ->Result.flatMap(graph => {
          // know pid already exist
          switch (graph->pathFromNode(pid)) {
          | Some(parentPathUp) =>
            let pidPathUp = parentPathUp->P.append(under);
            let tree =
              graph.tree->IDTree.addSubtree(id, pidPathUp, subtree.tree);
            let recalculatedPaths =
              tree
              ->IDTree.getChildPaths(pidPathUp, false) // dont want parent ID here
              ->Array.map(d => {
                  let i = fst(d)->I.convertChildToFocus;
                  let pth = snd(d);
                  /* %log.debug */
                  /* "got i: " ++ i->ID.toString ++ " - " ++ pth->P.toString; */
                  let dataWithPath = masterLookup->Map.getExn(i);
                  (i, {...dataWithPath, pathUp: pth});
                });

            Result.Ok({
              masterLookup: masterLookup->Map.mergeMany(recalculatedPaths),
              tree,
            });
          | None => Result.Error("moveSubtree failed to get parent path")
          }
        })
    | None => Result.Error("moveSubtree failed to get subtree")
    }
  | (false, _)
  | (_, false) => Result.Ok(graph)
  };
};

let map = (graph: t, f): t => {
  {
    ...graph,
    masterLookup:
      graph.masterLookup->Map.map(d => {...d, value: f(d.value)}),
  };
};

let updateChildren = (graph: t, id: ID.t, f: 'a => 'a): t => {
  switch (graph->subGraphForNode(id)) {
  | Some(subgraph) =>
    let idsToUpdate =
      subgraph.tree
      ->IDTree.children
      ->Map.keysToArray
      ->Array.map(I.convertChildToFocus)
      ->Array.map(id => {
          let d = subgraph.masterLookup->Map.getExn(id);
          /* [%log.debug "got id: " ++ id->ID.toString; ("", "")]; */
          (id, {...d, value: f(d.value)});
        });

    let masterLookup = graph.masterLookup->Map.mergeMany(idsToUpdate);
    {...graph, masterLookup};
  | None => graph
  };
};

let forEach = (graph: t, f: (ID.t, 'a) => unit): unit => {
  graph.masterLookup->Map.forEach((k, v) => {f(k, v.value)});
};

let keep = (graph: t, f: (ID.t, 'a) => bool): t => {
  // partition masterLookup into (keeping, discarding)
  // then remove from tree whats in the discard pile
  // but making sure that remaining nodes are all reachable from the root
  // then reform smaller master lookup by getting new paths
  let (willKeep, willDiscard) =
    graph.masterLookup
    ->Map.toArray
    ->Array.partition(kv => {f(fst(kv), snd(kv).value)});

  let lookup = ID.Map.make()->Map.mergeMany(willKeep);

  let tree =
    willDiscard->Array.reduce(
      graph.tree,
      (tree, idAndPath) => {
        let id = fst(idAndPath);
        let dataWithPath = snd(idAndPath);
        tree->IDTree.removeSubtree(
          dataWithPath.pathUp,
          id->I.convertFocusToChild,
        );
      },
    );

  let newChildPaths =
    tree
    ->IDTree.getAllPaths
    ->Array.map(d => {
        let i = fst(d)->I.convertChildToFocus;
        let pth = snd(d);
        let dataWithPath = lookup->Map.getExn(i);
        (i, {...dataWithPath, pathUp: pth});
      });

  let newGraph = empty();
  let masterLookup = newGraph.masterLookup->Map.mergeMany(newChildPaths);
  {masterLookup, tree};
};

let toKeyValueArray = (graph: t): array((ID.t, 'a)) => {
  graph.masterLookup
  ->Map.toArray
  ->Array.map(kv => (fst(kv), (snd(kv): dataWithPath).value));
};

let toArray = (graph: t): array('a) => {
  graph->toKeyValueArray->Array.map(d => snd(d));
};

}
