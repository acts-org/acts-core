import pytest
from pathlib import Path

import acts

from acts import MaterialMapJsonConverter, JsonMaterialDecorator


@pytest.mark.root
def test_material_root(conf_const):
    with pytest.raises(TypeError):
        acts.examples.RootMaterialDecorator()
    fileName = "blubb.root"
    try:
        conf_const(
            acts.examples.RootMaterialDecorator,
            level=acts.logging.INFO,
            fileName=fileName,
        )
    except RuntimeError as e:
        assert fileName in str(e)


def test_json_material_decorator():
    config = MaterialMapJsonConverter.Config()
    deco = JsonMaterialDecorator(
        rConfig=config,
        jFileName=str(
            Path(__file__).parent.parent.parent.parent
            / "Examples/Algorithms/MaterialMapping/share/odd-material-mapping-config.json"
        ),
        level=acts.logging.WARNING,
    )
