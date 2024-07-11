from schema import And, Optional, Schema

LBDatafile_schema = Schema(
    {
        Optional('type'): And(str, "LBDatafile", error="'LBDatafile' must be chosen."),
        Optional('metadata'): {
            Optional('type'): And(str, "LBDatafile", error="'LBDatafile' must be chosen."),
            Optional('rank'): int,
            Optional('shared_node'): {
                'id': int,
                'size': int,
                'rank': int,
                'num_nodes': int,
            },
            Optional('phases'): {
                Optional('count'): int,
                'skipped': {
                    'list': [int],
                    'range': [[int]],
                },
                'identical_to_previous': {
                    'list': [int],
                    'range': [[int]],
                },
            },
            Optional('attributes'): dict
        },
        'phases': [
            {
                'id': int,
                'tasks': [
                    {
                        'entity': {
                            Optional('collection_id'): int,
                            'home': int,
                            'id': int,
                            Optional('index'): [int],
                            'type': str,
                            'migratable': bool,
                            Optional('objgroup_id'): int
                        },
                        'node': int,
                        'resource': str,
                        Optional('subphases'): [
                            {
                                'id': int,
                                'time': float,
                            }
                        ],
                        'time': float,
                        Optional('user_defined'): dict,
                        Optional('attributes'): dict
                    },
                ],
                Optional('communications'): [
                    {
                        'type': str,
                        'to': {
                            'type': str,
                            'id': int,
                            Optional('home'): int,
                            Optional('collection_id'): int,
                            Optional('migratable'): bool,
                            Optional('index'): [int],
                            Optional('objgroup_id'): int,
                        },
                        'messages': int,
                        'from': {
                            'type': str,
                            'id': int,
                            Optional('home'): int,
                            Optional('collection_id'): int,
                            Optional('migratable'): bool,
                            Optional('index'): [int],
                            Optional('objgroup_id'): int,
                        },
                        'bytes': float
                    }
                ],
                Optional('user_defined'): dict
            },
        ]
    }
)
