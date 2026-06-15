#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "em_cmd_client_assoc_ctrl_req.h"

em_cmd_client_assoc_ctrl_req_t::em_cmd_client_assoc_ctrl_req_t(em_cmd_client_assoc_params_t params,
    dm_easy_mesh_t& dm, em_service_type_t svc)
{
    em_cmd_ctx_t ctx;

    m_type = em_cmd_type_client_assoc_ctrl_req;
    memcpy(&m_param.u.client_assoc_params, &params, sizeof(em_cmd_client_assoc_params_t));

    memset(reinterpret_cast<unsigned char *>(&m_orch_desc[0]), 0, EM_MAX_CMD * sizeof(em_orch_desc_t));

    m_orch_op_idx = 0;
    m_num_orch_desc = 1;
    m_orch_desc[0].op = dm_orch_type_client_assoc;
    m_orch_desc[0].submit = true;

    strncpy(m_name, "client_assoc", strlen("client_assoc") + 1);
    m_svc = svc;
    init(dm);

    memset(&ctx, 0, sizeof(em_cmd_ctx_t));
    ctx.type = m_orch_desc[0].op;
    m_data_model.set_cmd_ctx(&ctx);
}
