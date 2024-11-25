from locust import HttpUser, task, between

class UserBehavior(HttpUser):
    wait_time = between(1, 5)

    @task
    def index(self):
        self.client.get("/")

    @task
    def second_page(self):
        self.client.get("/page2.html")

    @task
    def nonexistent_page(self):
        with self.client.get("/nonexistent.html", catch_response=True) as response:
            if response.status_code == 404:
                response.success()

class WebsiteUser(HttpUser):
    tasks = [UserBehavior]
    wait_time = between(1, 5)
