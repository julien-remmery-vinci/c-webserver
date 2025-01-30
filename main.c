#include "server.h"

#include "jwt_middleware.h"
#include "root_routes.h"
#include "login_routes.h"
#include "dashboard_routes.h"

Ws_Router
setup_router()
{
    Ws_Router router = {0};
    // Initialize the map to store routes with a size of 10
    router.routes = hm_create(HM_DEFAULT_SIZE);

    Ws_router_handle(&router, "/", HTTP_METHOD_GET, route_get_root, NULL);
    Ws_router_handle(&router, "/index.js", HTTP_METHOD_GET, route_get_root_js, NULL);
    Ws_router_handle(&router, "/index.css", HTTP_METHOD_GET, route_get_root_css, NULL);
    Ws_router_handle(&router, "/favicon.ico", HTTP_METHOD_GET, route_get_favicon, NULL);
    Ws_router_handle(&router, "/dashboard", HTTP_METHOD_GET, route_get_dashboard, authorize);
    Ws_router_handle(&router, "/api/login", HTTP_METHOD_POST, route_post_login, NULL);
    return router;
}

int 
main(void)
{
    Ws_Config config = Ws_load_config_from_file(NULL);
    Ws_Router router = setup_router();
    Ws_Server server = Ws_server_setup(config, router);
    Ws_server_enable_logging(&server);
    return Ws_run_server(&server);
}