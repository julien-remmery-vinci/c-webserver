# Conrad
Simple C http server implementation.

# Uses
From sending files to building a REST API.

# Features

- **Routing**: Associate a path to an http method (ex: GET /), associate a function to execute when a request is made to this endpoint.
- **Middleware**: Execute one or multiple functions before executing the endpoint's main function. (Authorizations, ...)
- **JSON serialization / deserialization**: (using [Jacon](https://github.com/julien-remmery-vinci/Jacon))
- **JWT / API Key Auth**: (using [Toki](https://github.com/julien-remmery-vinci/Toki))
- **Server configuration**: via config file, server port, max simultaneous connections, max http request size, ... (If you do not provide your own configuration file, you must leave the default one in project root)

# About
This project was made to have a better understanding of how http servers work under the hood. It is not suitable for production. If you still plan to use Conrad for any reasons, feel free, if you need and/or want any changes made to this, feel free to submit your ideas.